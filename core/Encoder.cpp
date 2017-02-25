// -------------------------------------------------------------------------------------------------
//
// Copyright (C) Juniarto Saputra (juniarto.wongso@gmail.com). All rights reserved.
//
// This software, including documentation, is protected by copyright controlled by
// me. All rights are reserved. Copying, including reproducing, storing,
// adapting or translating, any or all of this material requires the prior written
// consent of Juniarto Saputra.
//
// -------------------------------------------------------------------------------------------------

#include <algorithm>

#include "Encoder.h"
#include "utils/FileSystemHelper.h"
#include "utils/WAVEHelper.h"

namespace core
{

namespace
{
const std::string EXT_WAV = ".WAV";
}

// -------------------------------------------------------------------------------------------------

Encoder::Encoder( InputAudioType input_type, OutputEncoderType output_type )
    : m_input_type( input_type )
    , m_output_type( output_type )
{
}

// -------------------------------------------------------------------------------------------------

Encoder::~Encoder( )
{
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
Encoder::scan_input_directory( const std::string& dir )
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

    if ( m_input_type == InputAudioType::INPUT_WAV )
    {
        files.erase( std::remove_if( files.begin( ), files.end( ),
                                     [ & ] ( const std::string& filename )
        {
            return ( !utils::WAVEHelper::validate( filename ) );
        } ), files.end( ) );
    }

    m_input_files = files;

    return common::ErrorCode::ERROR_NONE;
}

// -------------------------------------------------------------------------------------------------

const std::vector< std::string >&
Encoder::get_input_files() const
{
    return m_input_files;
}

// -------------------------------------------------------------------------------------------------

} // core