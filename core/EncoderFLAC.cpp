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

#include "EncoderFLAC.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

#include <FLAC/stream_encoder.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <memory>

namespace core
{

namespace
{
const std::string FLAC = "FLAC ";
const std::string OUTPUT_EXT = ".flac";
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;

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

} // namespace

// -------------------------------------------------------------------------------------------------

EncoderFLAC::EncoderFLAC(common::AudioFormatType input_type, uint16_t thread_number)
: Encoder(input_type, common::AudioFormatType::FLAC)
, m_encoder_version(FLAC + get_flac_version())
, m_thread_number(thread_number)
, m_cancelled(false)
{
}

// -------------------------------------------------------------------------------------------------

EncoderFLAC::~EncoderFLAC()
{
}

// -------------------------------------------------------------------------------------------------

std::string
EncoderFLAC::get_flac_version()
{
    return FLAC__VERSION_STRING;
}

// -------------------------------------------------------------------------------------------------

const std::string&
EncoderFLAC::get_encoder_version() const
{
    return m_encoder_version;
}

// -------------------------------------------------------------------------------------------------

void*
EncoderFLAC::processing_files(void* arg)
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
            FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
            if (!encoder)
            {
                throw std::runtime_error("Failed to create FLAC encoder");
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

            for (uint16_t ch = 0; ch < header.channels; ch++)
            {
                buffer[ch] = new FLAC__int32[total_samples];
            }

            // Deinterleave or copy data
            if (header.channels == 2)
            {
                for (uint32_t i = 0; i < total_samples; i++)
                {
                    buffer[0][i] = left[i];
                    buffer[1][i] = right[i];
                }
            }
            else if (header.channels == 1)
            {
                for (uint32_t i = 0; i < total_samples; i++)
                {
                    buffer[0][i] = left[i];
                }
            }

            // Open output file
            FILE* file = fopen(output_file.c_str(), "wb");
            if (!file)
            {
                throw std::runtime_error("Failed to open output file");
            }

            // Initialize encoder with callbacks
            FLAC__StreamEncoderInitStatus init_status =
                FLAC__stream_encoder_init_stream(encoder,
                                               write_callback,
                                               nullptr, // seek callback (not needed)
                                               nullptr, // tell callback (not needed)
                                               metadata_callback,
                                               file);

            if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
            {
                fclose(file);
                throw std::runtime_error("Failed to initialize FLAC encoder");
            }

            // Encode data
            if (!FLAC__stream_encoder_process(encoder,
                                             reinterpret_cast<const FLAC__int32* const*>(buffer.data()),
                                             total_samples))
            {
                utils::Helper::log(callback, thread_id, "Encoding error");
                FLAC__stream_encoder_finish(encoder);
                fclose(file);
                throw std::runtime_error("FLAC encoding failed");
            }

            // Finish encoding
            FLAC__stream_encoder_finish(encoder);
            FLAC__stream_encoder_delete(encoder);
            fclose(file);

            // Clean up buffer
            for (uint16_t ch = 0; ch < header.channels; ch++)
            {
                delete[] buffer[ch];
            }

            utils::Helper::log(callback, thread_id, "Finished: " + output_file);
        }
        catch (const std::exception& e)
        {
            fprintf(stderr, "Exception: %s\n", e.what());
            utils::Helper::log(callback, thread_id, std::string("Error: ") + e.what());
        }

        delete[] left;
        delete[] right;
    }

    pthread_exit(nullptr);
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
EncoderFLAC::start_encoding()
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
                                           EncoderFLAC::processing_files,
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
EncoderFLAC::cancel_encoding()
{
    m_cancelled = true;

    return common::ErrorCode::ERROR_CANCELLED;
}

// -------------------------------------------------------------------------------------------------

void
EncoderFLAC::on_encoding_status(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::string log = key + " " + value;
    m_status.emplace_back(log);

    std::cout << log << std::endl;
}

// -------------------------------------------------------------------------------------------------

} // core
