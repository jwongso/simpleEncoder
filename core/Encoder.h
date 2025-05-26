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

#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <atomic>

#include "common/Common.h"
#include "utils/FileSystemHelper.h"
#include "utils/WaveHeader.h"
#include "utils/WaveFileWrapper.h"

static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;

namespace core
{
class Encoder
{
public:
    Encoder() = delete;

    virtual ~Encoder()
    {}

    common::ErrorCode scan_input_directory(const std::string& dir)
    {
        utils::FileSystemHelper fs;
        if ( !fs.directory_exists( dir ) )
        {
            return common::ErrorCode::ERROR_NOT_FOUND;
        }

        std::vector< std::string > files;

        if ( !fs.get_file_paths( dir, files ) )
        {
            return common::ErrorCode::ERROR_NOT_FOUND;
        }

        if ( m_input_type == common::AudioFormatType::WAV )
        {
            files.erase( std::remove_if( files.begin( ), files.end( ),
                                        [ & ] ( const std::string& filename )
            {
                utils::WaveHeader header;
                return ( !utils::WaveFileWrapper::validate( filename, header ) );
            } ), files.end( ) );
        }

        m_input_files = files;

        return common::ErrorCode::ERROR_NONE;
    }

    void set_output_directory(const std::string& dir)
    {
        m_output_directory = dir;
    }

    const std::string& get_output_directory() const
    {
        return m_output_directory;
    }

    const std::vector<std::string>& get_input_files() const
    {
        return m_input_files;
    }

    virtual const std::string& get_encoder_version() const = 0;

    common::ErrorCode start_encoding() {
        if (m_input_files.empty()) {
            return common::ErrorCode::ERROR_NOT_FOUND;
        }

        m_cancelled = false;
        m_to_be_encoded_files.clear();
        for (const auto& file : m_input_files) {
            m_to_be_encoded_files[file] = false;
        }

        std::vector<pthread_t> threads(m_thread_number);
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

        // Thread arguments
        struct ThreadTask {
            Encoder* encoder;
            std::string output_dir;
            uint32_t thread_id;
        };

        auto worker = [](void* arg) -> void* {
            ThreadTask* task = static_cast<ThreadTask*>(arg);
            std::string input_file;

            // Thread-safe file acquisition
            pthread_mutex_lock(&process_mutex);
            for (auto& [file, processed] : task->encoder->m_to_be_encoded_files) {
                if (!processed) {
                    input_file = file;
                    processed = true;
                    break;
                }
            }
            pthread_mutex_unlock(&process_mutex);

            if (!input_file.empty()) {
                task->encoder->process_single_file(
                    input_file,
                    task->output_dir,
                    task->thread_id,
                    [task](auto&& k, auto&& v) {
                        task->encoder->on_encoding_status(k, v);
                    }
                );
            }
            return nullptr;
        };

        std::vector<ThreadTask> tasks(m_thread_number);
        for (uint32_t i = 0; i < m_thread_number; ++i) {
            tasks[i] = {
                .encoder = this,
                .output_dir = m_output_directory,
                .thread_id = i + 1
            };

            if (pthread_create(&threads[i], &thread_attr, worker, &tasks[i]) != 0) {
                return common::ErrorCode::ERROR_PTHREAD_CREATE;
            }
        }

        pthread_attr_destroy(&thread_attr);
        for (auto& thread : threads) {
            pthread_join(thread, nullptr);
        }

        return common::ErrorCode::ERROR_NONE;
    }

    virtual common::ErrorCode cancel_encoding()
    {
        m_cancelled = true;
        return common::ErrorCode::ERROR_CANCELLED;
    }

protected:
    Encoder(common::AudioFormatType input_type,
            common::AudioFormatType output_type,
            const std::string& encoder_version = "",
            uint16_t thread_number = 1,
            bool cancelled = false,
            bool verbose = false)
    : m_input_type( input_type )
    , m_output_type( output_type )
    , m_encoder_version(encoder_version)
    , m_thread_number(thread_number)
    , m_cancelled(cancelled)
    , m_verbose(verbose)
    {}

    struct EncoderThreadArg
    {
        uint32_t thread_id;
        std::map<std::string, bool>* input_files;
        std::atomic<bool>* cancelled;
        std::function<void(const std::string&, const std::string&)> callback;
        bool verbose;
        std::string output_directory;
        Encoder* encoder;  // Pointer to the encoder instance
    };

    virtual common::ErrorCode process_single_file(
        const std::string& input_file,
        const std::string& output_dir,
        uint32_t thread_id,
        const std::function<void(const std::string&, const std::string&)>& status_cb) = 0;

    void on_encoding_status(const std::string& key, const std::string& value)
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        std::string log = key + " " + value;
        m_status.emplace_back(log);

        if (m_verbose)
        {
            std::cout << log << std::endl;
        }
    }

protected:
    common::AudioFormatType m_input_type;
    common::AudioFormatType m_output_type;
    std::string m_input_directory;
    std::string m_output_directory;
    std::vector<std::string> m_input_files;
    std::map< std::string, bool > m_to_be_encoded_files;
    std::vector<std::string> m_status;
    std::string m_encoder_version;
    uint16_t m_thread_number;
    std::atomic<bool> m_cancelled;
    std::mutex m_mutex;
    bool m_verbose;
};

} // core

#endif // ENCODER_H
