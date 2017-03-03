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

#include "WaveFileWrapper.h"
#include "FileSystemHelper.h"
#include "Helper.h"

#include <cstring>
#include <fstream>

namespace utils
{

namespace
{

const char* RIFF                = "RIFF";
const char* WAVE                = "WAVE";
const char* FMT                 = "fmt ";
const char* DATA                = "data";
const char* LIST                = "LIST";

const uint16_t MIN_HEADER_SIZE  = 44;

}

// -------------------------------------------------------------------------------------------------

WaveFileWrapper::WaveFileWrapper( const std::string& filename )
    : m_filename( filename )
{
    m_valid = validate( m_filename, m_header );
}

// -------------------------------------------------------------------------------------------------

bool
WaveFileWrapper::is_valid( ) const
{
    return m_valid;
}

// -------------------------------------------------------------------------------------------------

bool
WaveFileWrapper::get_wave_data( WaveHeader& header_data, int16_t*& left, int16_t*& right ) const
{
    if ( !m_valid )
    {
        return false;
    }

    uint32_t samples = m_header.data_size / m_header.block_align;
    std::ifstream input( m_filename, std::ifstream::in | std::ifstream::binary );

    if ( !input.is_open( ) )
    {
        return false;
    }

    header_data = m_header;
    input.seekg( sizeof( header_data ) );
    left = new int16_t[ header_data.data_size / header_data.channels / sizeof( int16_t ) ];

    if ( header_data.channels == 1 )
    {
        input.read( ( char* )left, header_data.block_align * samples );
    }
    else
    {
        right = new int16_t[ header_data.data_size / header_data.channels / sizeof( int16_t ) ];

        for ( int i = 0; i < samples; i++ )
        {
            input.read( (char*)&left[ i ], header_data.block_align / header_data.channels );

            input.read( (char*)&right[ i ], header_data.block_align / header_data.channels );
        }
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

bool
WaveFileWrapper::validate( const std::string& filename, WaveHeader& header )
{
    std::vector< uint8_t > contents;

    if ( !FileSystemHelper::read_binary_file( filename, contents ) )
    {
        return false;
    }

    if ( contents.size( ) < MIN_HEADER_SIZE )
    {
        return false;
    }

    uint32_t pos = 0;
    Helper::read_as_chars( contents, pos, 4, header.riff );

    if ( strncmp( header.riff, RIFF, strlen( RIFF ) ) != 0 )
    {
        return false;
    }

    pos += sizeof( header.riff ); // 4
    header.file_length = Helper::read_as_uint32_little( contents, pos );
    pos += sizeof( uint32_t ); // 4
    Helper::read_as_chars( contents, 8, 4, header.wave );

    if ( strncmp( header.wave, WAVE, strlen( WAVE ) ) != 0 )
    {
        return false;
    }

    pos += sizeof( header.wave ); // 4

    bool found_fmt = false;
    uint32_t pos_data = 0;

    while ( !found_fmt && pos + 4 < contents.size( ) )
    {
        Helper::read_as_chars( contents, pos, 4, header.fmt );
        pos += 4;

        if ( strncmp( header.fmt, FMT, strlen( FMT ) ) != 0 )
        {
            return false;
        }

        found_fmt = true;

        header.chunk_size = Helper::read_as_uint32_little( contents, pos );
        pos += sizeof( uint32_t ); // 4
        pos_data = pos + header.chunk_size;

        header.format = Helper::read_as_uint16( contents, pos );
        pos += sizeof( uint16_t ); // 2

        header.channels = Helper::read_as_uint16( contents, pos );
        pos += sizeof( uint16_t ); // 2

        header.sampes_per_sec = Helper::read_as_uint32_little( contents, pos );
        pos += sizeof( uint32_t ); // 4

        header.bytes_per_sec = Helper::read_as_uint32_little( contents, pos );
        pos += sizeof( uint32_t ); // 4

        header.block_align = Helper::read_as_uint16( contents, pos );
        pos += sizeof( uint16_t ); // 2

        header.bits_per_sample = Helper::read_as_uint16( contents, pos );
        pos += sizeof( uint16_t ); // 2

        break;
    }

    if ( pos != pos_data )
    {
        pos = pos_data;
    }

    bool found_data = false;

    while ( !found_data && pos + 4 < contents.size( ) )
    {
        Helper::read_as_chars( contents, pos, 4, header.data );
        pos += 4;

        if ( strncmp( header.data, LIST, strlen( LIST ) ) == 0 )
        {
            uint32_t info_size = Helper::read_as_uint32_little( contents, pos );
            pos += info_size;

            continue;
        }

        if ( strncmp( header.data, DATA, strlen( DATA ) ) == 0 )
        {
            found_data = true;

            header.data_size = Helper::read_as_uint32_little( contents, pos );
            pos += sizeof( uint32_t );

            break;
        }
    }

    if ( !found_data )
    {
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

} // utils
