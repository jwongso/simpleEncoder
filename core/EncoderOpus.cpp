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

#include "EncoderOpus.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

#include <opus/opus.h>
#include <opus/opusenc.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <memory>

namespace core
{

namespace
{
const std::string OPUS = "Opus ";
const std::string OUTPUT_EXT = ".opus";
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;

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

} // namespace

// -------------------------------------------------------------------------------------------------

EncoderOpus::EncoderOpus(common::AudioFormatType input_type, uint16_t thread_number)
: Encoder(input_type, common::AudioFormatType::OPUS)
, m_encoder_version(OPUS + get_opus_version())
, m_thread_number(thread_number)
, m_cancelled(false)
{
}

// -------------------------------------------------------------------------------------------------

EncoderOpus::~EncoderOpus()
{
}

// -------------------------------------------------------------------------------------------------

std::string
EncoderOpus::get_opus_version()
{
    return opus_get_version_string();
}

// -------------------------------------------------------------------------------------------------

const std::string&
EncoderOpus::get_encoder_version() const
{
    return m_encoder_version;
}

// -------------------------------------------------------------------------------------------------

void*
EncoderOpus::processing_files(void* arg)
{
    auto* thread_arg = static_cast<EncoderThreadArg*>(arg);
    const uint32_t thread_id = thread_arg->thread_id;
    const auto& callback = thread_arg->callback;

    while (true)
    {
        std::string input_file;

        // Lock and retrieve unprocessed file
        pthread_mutex_lock(&process_mutex);
        for (auto& [file, processed] : *thread_arg->input_files)
        {
            if (!processed)
            {
                input_file = file;
                processed = true;
                break;
            }
        }

        bool cancelled = *thread_arg->cancelled;
        pthread_mutex_unlock(&process_mutex);

        // Check for cancellation
        if (cancelled)
        {
            utils::Helper::log(callback, thread_id, "Cancelled");
            break;
        }

        if (input_file.empty())
        {
            break;  // No more files
        }

        utils::Helper::log(callback, thread_id, "Processing " + input_file);

        std::string output_file = utils::Helper::generate_output_file(input_file, OUTPUT_EXT);
        utils::WaveHeader header;
        utils::WaveFileWrapper wave(input_file);

        if (!wave.is_valid())
        {
            fprintf(stderr, "Invalid wave file: %s\n", input_file.c_str());
            utils::Helper::log(callback, thread_id, "Invalid wave file: " + input_file);
            continue;
        }

        int16_t* left = nullptr;
        int16_t* right = nullptr;

        if (!wave.get_wave_data(header, left, right))
        {
            fprintf(stderr, "Error reading PCM data from %s\n", input_file.c_str());
            utils::Helper::log(callback, thread_id, "Error reading PCM data");
            delete[] left;
            delete[] right;
            continue;
        }

        try
        {
            // Interleave stereo or copy mono
            std::vector<int16_t> pcm_data(header.data_size / sizeof(int16_t));
            if (header.channels == 2)
            {
                for (uint32_t i = 0; i < header.data_size / (2 * sizeof(int16_t)); i++)
                {
                    pcm_data[i * 2] = left[i];
                    pcm_data[i * 2 + 1] = right[i];
                }
            }
            else
            {
                std::memcpy(pcm_data.data(), left, header.data_size);
            }

            std::vector<int16_t> resampled_pcm;
            resampleTo48kHz(pcm_data.data(),
                            header.data_size / (header.channels * sizeof(int16_t)),
                            header.sample_rate,
                            header.channels,
                            resampled_pcm);

            OggOpusComments* comments = ope_comments_create();
            ope_comments_add(comments, "ENCODER", "MyAudioEncoder");

            fprintf(stderr, "Creating encoder with: output=%s, rate=%d, channels=%d\n",
                    output_file.c_str(), 48000, header.channels);

            int opus_err;
            OggOpusEnc* enc = ope_encoder_create_file(
                output_file.c_str(),
                comments,
                48000,
                header.channels,
                0,
                &opus_err
                );

            if (!enc)
            {
                fprintf(stderr, "Error creating encoder: %s\n", ope_strerror(opus_err));
                utils::Helper::log(callback, thread_id, "Failed to create encoder");
                ope_comments_destroy(comments);
                throw std::runtime_error("Encoder creation failed");
            }

            ope_encoder_ctl(enc, OPUS_SET_BITRATE(128000));
            ope_encoder_ctl(enc, OPUS_SET_COMPLEXITY(10));

            const int frame_size = 960;
            const uint32_t total_samples = resampled_pcm.size() / header.channels;

            for (uint32_t i = 0; i < total_samples; i += frame_size)
            {
                uint32_t current_frame_size = std::min(frame_size, (int)(total_samples - i));
                if (ope_encoder_write(enc, &resampled_pcm[i * header.channels], current_frame_size) != OPE_OK)
                {
                    utils::Helper::log(callback, thread_id, "Encoding error");
                    throw std::runtime_error("Encoding error");
                }
            }

            ope_encoder_destroy(enc);
            ope_comments_destroy(comments);
            utils::Helper::log(callback, thread_id, "Finished: " + output_file);
        }
        catch (const std::exception& e)
        {
            fprintf(stderr, "Exception: %s\n", e.what());
        }

        delete[] left;
        delete[] right;
    }

    pthread_exit(nullptr);
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
EncoderOpus::start_encoding()
{
    if (m_input_files.empty())
    {
        return common::ERROR_NOT_FOUND;
    }

    m_to_be_encoded_files.clear();

    for (const auto& file : m_input_files)
    {
        m_to_be_encoded_files[file] = false;
    }

    pthread_t threads[m_thread_number];
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    for (int i = 0; i < m_thread_number; i++)
    {
        EncoderThreadArg thread_arg;
        thread_arg.thread_id = (i + 1);
        thread_arg.input_files = &m_to_be_encoded_files;
        thread_arg.cancelled = &m_cancelled;

        auto callback = [this](const std::string& key, const std::string& value)
        {
            on_encoding_status(key, value);
        };

        thread_arg.callback = std::move(callback);

        RETURN_ERR_IF_ERROR(pthread_create(&threads[i],
                                           &thread_attr,
                                           EncoderOpus::processing_files,
                                           (void*)&thread_arg),
                            common::ErrorCode::ERROR_PTHREAD_CREATE);
    }

    pthread_attr_destroy(&thread_attr);

    for (int i = 0; i < m_thread_number; i++)
    {
        RETURN_ERR_IF_ERROR(pthread_join(threads[i], NULL),
                            common::ErrorCode::ERROR_PTHREAD_JOIN);
    }

    #ifdef ENABLE_LOG
    std::ofstream ofs(ENCODER_LOG_FILE);
    if (ofs.is_open())
    {
        for (const auto& status : m_status)
        {
            ofs << status << std::endl;
        }
    }
    ofs.close();
    #endif

    m_cancelled = false;

    return common::ErrorCode::ERROR_NONE;
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
EncoderOpus::cancel_encoding()
{
    m_cancelled = true;

    return common::ErrorCode::ERROR_CANCELLED;
}

// -------------------------------------------------------------------------------------------------

void
EncoderOpus::on_encoding_status(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::string log = key + " " + value;
    m_status.emplace_back(log);

    std::cout << log << std::endl;
}

// -------------------------------------------------------------------------------------------------

} // core
