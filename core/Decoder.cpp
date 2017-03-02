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

#include <algorithm>

#include "Decoder.h"
#include "utils/FileSystemHelper.h"
#include "utils/WaveFileWrapper.h"

namespace core
{

// -------------------------------------------------------------------------------------------------

Decoder::Decoder( common::AudioFormatType input_type, common::AudioFormatType output_type )
    : m_input_type( input_type )
    , m_output_type( output_type )
{
}

// -------------------------------------------------------------------------------------------------

Decoder::~Decoder( )
{
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
Decoder::scan_input_directory( const std::string& dir )
{
    if ( !utils::FileSystemHelper::directory_exists( dir ) )
    {
        return common::ErrorCode::ERROR_NOT_FOUND;
    }

    std::vector< std::string > files;

    if ( !utils::FileSystemHelper::get_file_paths( dir, files ) )
    {
        return common::ErrorCode::ERROR_NOT_FOUND;
    }

    if ( m_input_type == common::AudioFormatType::MP3 )
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

// -------------------------------------------------------------------------------------------------

const std::vector< std::string >&
Decoder::get_input_files() const
{
    return m_input_files;
}

// -------------------------------------------------------------------------------------------------

} // core
