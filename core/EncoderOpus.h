// -------------------------------------------------------------------------------------------------
//
// Copyright (C) all of the contributors. All rights reserved.
//
// This software, including documentation, is protected by copyright controlled by
// contributors. All rights are reserved. Copying, including reproducing, storing,
// adapting or translating, any or all of this material requires the prior written
// consent of all contributors.
//
// -------------------------------------------------------------------------------------------------

#ifndef ENCODER_OPUS_H
#define ENCODER_OPUS_H

#include "Encoder.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

#include <opus/opus.h>
#include <opus/opusenc.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <map>

const std::string OPUS = "Opus ";
const std::string OPUS_EXT = ".opus";

// Helper for resampling (simplified linear interpolator)
void resampleTo48kHz(const int16_t* input, int input_samples, int input_rate, int channels,
                     std::vector<int16_t>& output) {
    if (input_rate == 48000) {
        output.assign(input, input + input_samples * channels);
        return;
    }

    double ratio = 48000.0 / input_rate;
    int output_samples = static_cast<int>(input_samples * ratio);
    output.resize(output_samples * channels);

    for (int i = 0; i < output_samples; ++i) {
        double src_pos = i / ratio;
        int src_idx = static_cast<int>(src_pos);
        float frac = static_cast<float>(src_pos - src_idx);

        for (int c = 0; c < channels; ++c) {
            int idx = src_idx * channels + c;
            int next_idx = (src_idx + 1 < input_samples) ? (idx + channels) : idx;

            int16_t a = input[idx];
            int16_t b = input[next_idx];
            output[i * channels + c] = static_cast<int16_t>(a + frac * (b - a));
        }
    }
}

namespace core
{

class EncoderOpus : public Encoder
{
public:
    EncoderOpus( common::AudioFormatType input_type, uint16_t thread_number, bool m_verbose )
    : Encoder(input_type,
          common::AudioFormatType::OPUS,
          OPUS + get_opus_version(),
          thread_number,
          false,
          m_verbose)
    {
    }

    virtual ~EncoderOpus( ) {}

    const std::string& get_encoder_version( ) const
    {
        return m_encoder_version;
    }

private:
    virtual common::ErrorCode process_single_file(
        const std::string& input_file,
        const std::string& output_dir,
        uint32_t thread_id,
        const std::function<void(const std::string&, const std::string&)>& status_cb) override
    {
        status_cb("Thread " + std::to_string(thread_id), "Processing " + input_file);

        utils::WaveFileWrapper wave(input_file);
        if (!wave.is_valid()) {
            status_cb("Error", "Invalid WAV file: " + input_file);
            return common::ErrorCode::ERROR_WAV_INVALID;
        }

        // Generate output file with output directory
        size_t last_slash = input_file.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos)
                            ? input_file
                            : input_file.substr(last_slash + 1);

        std::string output_file = output_dir + "/" +
                                utils::Helper::generate_output_file(filename, OPUS_EXT);

        utils::WaveHeader header;
        int16_t* left = nullptr;
        int16_t* right = nullptr;

        if (!wave.get_wave_data(header, left, right)) {
            if (m_verbose) {
                fprintf(stderr, "Error reading PCM data from %s\n", input_file.c_str());
            }
            status_cb("Error", "Failed to read PCM data: " + input_file);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_READ_FILE;
        }

        try {
            // Interleave stereo or copy mono
            std::vector<int16_t> pcm_data(header.data_size / sizeof(int16_t));
            if (header.channels == 2) {
                for (uint32_t i = 0; i < header.data_size / (2 * sizeof(int16_t)); i++) {
                    pcm_data[i * 2] = left[i];
                    pcm_data[i * 2 + 1] = right[i];
                }
            } else {
                std::memcpy(pcm_data.data(), left, header.data_size);
            }

            std::vector<int16_t> resampled_pcm;
            resampleTo48kHz(pcm_data.data(),
                            header.data_size / (header.channels * sizeof(int16_t)),
                            header.sample_rate,
                            header.channels,
                            resampled_pcm);

            OggOpusComments* comments = ope_comments_create();
            if (!comments) {
                throw std::runtime_error("Failed to create Opus comments");
            }
            ope_comments_add(comments, "ENCODER", "MyAudioEncoder");

            if (m_verbose) {
                fprintf(stderr, "Creating encoder with: output=%s, rate=%d, channels=%d\n",
                    output_file.c_str(), 48000, header.channels);
            }

            int opus_err;
            OggOpusEnc* enc = ope_encoder_create_file(
                output_file.c_str(),
                comments,
                48000,
                header.channels,
                0,
                &opus_err
            );

            if (!enc) {
                if (m_verbose) {
                    fprintf(stderr, "Error: Failed to create Opus encoder: %s\n", ope_strerror(opus_err));
                }
                status_cb("Error", "Failed to create encoder: " + input_file);
                ope_comments_destroy(comments);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_OPUS;
            }

            ope_encoder_ctl(enc, OPUS_SET_BITRATE(128000));
            ope_encoder_ctl(enc, OPUS_SET_COMPLEXITY(10));

            const int frame_size = 960;
            const uint32_t total_samples = resampled_pcm.size() / header.channels;

            for (uint32_t i = 0; i < total_samples; i += frame_size) {
                uint32_t current_frame_size = std::min(frame_size, (int)(total_samples - i));
                if (ope_encoder_write(enc, &resampled_pcm[i * header.channels], current_frame_size) != OPE_OK) {
                    if (m_verbose) {
                        fprintf(stderr, "Error: Opus encoding failed\n");
                    }
                    status_cb("Error", "Encoding error: " + input_file);
                    ope_encoder_destroy(enc);
                    ope_comments_destroy(comments);
                    delete[] left;
                    delete[] right;
                    return common::ErrorCode::ERROR_OPUS;
                }
            }

            ope_encoder_destroy(enc);
            ope_comments_destroy(comments);
            delete[] left;
            delete[] right;

            status_cb("Complete", "Output: " + output_file);
            return common::ErrorCode::ERROR_NONE;
        }
        catch (const std::exception& e) {
            if (m_verbose) {
                fprintf(stderr, "Exception: %s\n", e.what());
            }
            status_cb("Error", "Exception: " + std::string(e.what()));
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_OPUS;
        }
    }

    static std::string get_opus_version( )
    {
        return opus_get_version_string();
    }
};

} // core

#endif // ENCODER_OPUS_H
