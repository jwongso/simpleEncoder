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

#ifndef DECODER_H
#define DECODER_H

#include <string>
#include <vector>
#include <algorithm>

#include "common/Common.h"
#include "utils/FileSystemHelper.h"
#include "utils/Mp3Header.h"
#include "utils/Mp3FileWrapper.h"

namespace core
{

class Decoder
{
public:
    Decoder( ) = delete;

    virtual ~Decoder( )
    {}

    common::ErrorCode scan_input_directory( const std::string& dir )
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

        if ( m_input_type == common::AudioFormatType::MP3 )
        {
            files.erase( std::remove_if( files.begin( ), files.end( ),
                                        [ & ] ( const std::string& filename )
            {
                std::vector< utils::ID3Tag > tags;
                utils::Mp3Header header;
                return ( !utils::Mp3FileWrapper::validate( filename, tags, header ) );
            } ), files.end( ) );
        }

        m_input_files = files;

        return common::ErrorCode::ERROR_NONE;
    }

    const std::vector< std::string >& get_input_files( ) const
    {
        return m_input_files;
    }

    virtual common::ErrorCode start_decoding( ) = 0;

    virtual common::ErrorCode cancel_decoding( ) = 0;

protected:
    Decoder( common::AudioFormatType input_type, common::AudioFormatType output_type )
    : m_input_type( input_type )
    , m_output_type( output_type )
    {}

protected:
    common::AudioFormatType m_input_type;
    common::AudioFormatType m_output_type;
    std::string m_input_directory;
    std::vector< std::string > m_input_files;
};

} // core

#endif // DECODER_H
