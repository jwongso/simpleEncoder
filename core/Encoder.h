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
    virtual common::ErrorCode start_encoding() = 0;
    virtual common::ErrorCode cancel_encoding() = 0;

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
