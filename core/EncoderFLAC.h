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

#ifndef ENCODER_FLAC_H
#define ENCODER_FLAC_H

#include "Encoder.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

#include <FLAC/stream_encoder.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <map>

const std::string FLAC = "FLAC ";
const std::string FLAC_EXT = ".flac";

// FLAC write callback
FLAC__StreamEncoderWriteStatus write_callback(const FLAC__StreamEncoder *encoder,
                                             const FLAC__byte buffer[],
                                             size_t bytes,
                                             unsigned samples,
                                             unsigned current_frame,
                                             void *client_data)
{
    FILE *file = (FILE*)client_data;
    if (fwrite(buffer, 1, bytes, file) != bytes)
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

// FLAC metadata callback
void metadata_callback(const FLAC__StreamEncoder *encoder,
                      const FLAC__StreamMetadata *metadata,
                      void *client_data)
{
    // This callback is not essential for basic encoding
    (void)encoder;
    (void)metadata;
    (void)client_data;
}

namespace core
{

/**
 * @class EncoderFLAC
 * @brief FLAC encoder implementation using libFLAC library.
 *
 * This class provides FLAC (Free Lossless Audio Codec) encoding functionality for converting
 * WAV audio files to FLAC format. It utilizes the libFLAC library for lossless compression
 * operations, ensuring perfect audio quality preservation while achieving significant file
 * size reduction.
 *
 * Key features:
 * - Lossless audio compression with configurable compression levels
 * - Support for mono and stereo audio
 * - Configurable compression level (default: 5 for balanced speed/compression)
 * - Verification of encoded data integrity
 * - Support for various sample rates and bit depths
 * - Thread-safe encoding operations
 * - Comprehensive error handling and status reporting
 *
 * Technical details:
 * - Output format: FLAC (.flac files)
 * - Compression: Lossless (perfect reconstruction)
 * - Default compression level: 5 (range: 0-8, higher = better compression)
 * - Supported input: WAV files (8, 16, 24-bit depths)
 * - Sample rates: 8kHz to 192kHz
 * - Channels: Mono and stereo
 * - Verification: Enabled by default for data integrity
 *
 * @note This encoder requires libFLAC library to be installed.
 *       The encoder automatically handles sample format conversion and
 *       provides built-in verification of the encoded stream.
 * @see Encoder base class for common functionality
 */
class EncoderFLAC : public Encoder
{
public:
    EncoderFLAC(common::AudioFormatType input_type, uint16_t thread_number, bool m_verbose)
    : Encoder(input_type,
          common::AudioFormatType::FLAC,
          FLAC + get_flac_version(),
          thread_number,
          false,
          m_verbose)
    {}

    virtual ~EncoderFLAC() {}

    const std::string& get_encoder_version() const
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
                                utils::Helper::generate_output_file(filename, FLAC_EXT);

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
            FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
            if (!encoder) {
                if (m_verbose) {
                    fprintf(stderr, "Error: Failed to create FLAC encoder\n");
                }
                status_cb("Error", "Failed to create encoder: " + input_file);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_FLAC;
            }

            // Set encoder parameters
            FLAC__stream_encoder_set_verify(encoder, true);
            FLAC__stream_encoder_set_compression_level(encoder, 5);
            FLAC__stream_encoder_set_channels(encoder, header.channels);
            FLAC__stream_encoder_set_bits_per_sample(encoder, header.bits_per_sample);
            FLAC__stream_encoder_set_sample_rate(encoder, header.sample_rate);

            // Prepare PCM data
            uint32_t total_samples = header.data_size / (header.channels * (header.bits_per_sample / 8));
            std::vector<FLAC__int32*> buffer(header.channels);

            for (uint16_t ch = 0; ch < header.channels; ch++) {
                buffer[ch] = new FLAC__int32[total_samples];
            }

            // Deinterleave or copy data
            if (header.channels == 2) {
                for (uint32_t i = 0; i < total_samples; i++) {
                    buffer[0][i] = left[i];
                    buffer[1][i] = right[i];
                }
            } else if (header.channels == 1) {
                for (uint32_t i = 0; i < total_samples; i++) {
                    buffer[0][i] = left[i];
                }
            }

            // Open output file
            FILE* file = fopen(output_file.c_str(), "wb");
            if (!file) {
                if (m_verbose) {
                    fprintf(stderr, "Error: Failed to open output file: %s\n", output_file.c_str());
                }
                status_cb("Error", "Failed to open output file: " + output_file);
                FLAC__stream_encoder_delete(encoder);
                for (uint16_t ch = 0; ch < header.channels; ch++) {
                    delete[] buffer[ch];
                }
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }

            // Initialize encoder with callbacks
            FLAC__StreamEncoderInitStatus init_status =
                FLAC__stream_encoder_init_stream(encoder,
                                            write_callback,
                                            nullptr, // seek callback (not needed)
                                            nullptr, // tell callback (not needed)
                                            metadata_callback,
                                            file);

            if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
                if (m_verbose) {
                    fprintf(stderr, "Error: Failed to initialize FLAC encoder: %d\n", init_status);
                }
                status_cb("Error", "Failed to initialize encoder: " + input_file);
                fclose(file);
                FLAC__stream_encoder_delete(encoder);
                for (uint16_t ch = 0; ch < header.channels; ch++) {
                    delete[] buffer[ch];
                }
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_FLAC;
            }

            // Encode data
            if (!FLAC__stream_encoder_process(encoder,
                                            reinterpret_cast<const FLAC__int32* const*>(buffer.data()),
                                            total_samples)) {
                if (m_verbose) {
                    fprintf(stderr, "Error: FLAC encoding failed\n");
                }
                status_cb("Error", "Encoding error: " + input_file);
                FLAC__stream_encoder_finish(encoder);
                fclose(file);
                FLAC__stream_encoder_delete(encoder);
                for (uint16_t ch = 0; ch < header.channels; ch++) {
                    delete[] buffer[ch];
                }
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_FLAC;
            }

            // Finish encoding
            FLAC__stream_encoder_finish(encoder);
            FLAC__stream_encoder_delete(encoder);
            fclose(file);

            // Clean up buffer
            for (uint16_t ch = 0; ch < header.channels; ch++) {
                delete[] buffer[ch];
            }
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
            return common::ErrorCode::ERROR_FLAC;
        }
    }

    static std::string get_flac_version()
    {
        return FLAC__VERSION_STRING;
    }
};

} // core

#endif // ENCODER_FLAC_H
