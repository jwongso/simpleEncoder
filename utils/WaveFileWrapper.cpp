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

#include "WAVEHelper.h"
#include "FileSystemHelper.h"

#include <cstring>
#include <fstream>

namespace utils
{

namespace
{

const char* RIFF    = "RIFF";
const char* WAVE    = "WAVE";
const char* FMT     = "fmt ";
const char* DATA    = "data";
const char* LIST    = "LIST";

// -------------------------------------------------------------------------------------------------

void
read_as_chars( const std::vector< uint8_t >& input, uint32_t pos, uint32_t length, char* target )
{
    if ( sizeof( target ) / sizeof( char ) < length )
    {
        return;
    }

    std::copy( input.begin( ) + pos, input.begin( ) + ( pos + length ), target );
}

uint32_t
read_as_uint32( const std::vector< uint8_t >& input, uint32_t pos )
{
    return ( input[ pos ] |
           ( input[ pos + 1 ] << 8 ) |
           ( input[ pos + 2 ] << 16 ) |
           ( input[ pos + 3 ] << 24 ) );
}

uint16_t
read_as_uint16( const std::vector< uint8_t >& input, uint32_t pos )
{
    return ( input[ pos ] << 0 ) | ( input[ pos + 1 ] << 8 );
}

template< class T >
std::vector< T >
read_as_chars( const std::vector< T >& input, uint32_t pos, uint32_t length )
{
    std::vector< T > output( input.begin( ) + pos, input.begin( ) + ( pos + length ) );
    return output;
}

}

// -------------------------------------------------------------------------------------------------

bool
WAVEHelper::validate( const std::string& filename )
{
    WAVEHeader header;
    uint32_t pos = 0;

    if ( get_wave_header( filename, header, pos ) )
    {
        // All WAVE ids should exist ("RIFF", "WAVE", "fmt " and "data").
        return ( strncmp( header.riff, RIFF, strlen( RIFF ) ) == 0 &&
                 strncmp( header.wave, WAVE, strlen( WAVE ) ) == 0 &&
                 strncmp( header.fmt, FMT, strlen( FMT ) ) == 0 &&
                 strncmp( header.data, DATA, strlen( DATA ) ) == 0 );
    }

    return false;
}

// -------------------------------------------------------------------------------------------------

bool
WAVEHelper::get_wave_header( const std::string& filename,
                             WAVEHeader& header_data,
                             uint32_t& last_pos )
{
    std::vector< uint8_t > contents;

    if ( !FileSystemHelper::read_binary_file( filename, contents ) )
    {
        return false;
    }

    uint32_t pos = 0;
    read_as_chars( contents, pos, 4, header_data.riff );
    pos += sizeof( header_data.riff ); // 4
    header_data.file_length = read_as_uint32( contents, pos );
    pos += sizeof( uint32_t ); // 4
    read_as_chars( contents, 8, 4, header_data.wave );
    pos += sizeof( header_data.wave ); // 4

    bool found_fmt = false;
    uint32_t pos_data = 0;

    while ( !found_fmt && pos + 4 < contents.size( ) )
    {
        read_as_chars( contents, pos, 4, header_data.fmt );
        pos += 4;

        if ( strncmp( header_data.fmt, FMT, strlen( FMT ) ) == 0 )
        {
            found_fmt = true;

            header_data.chunk_size = read_as_uint32( contents, pos );
            pos += sizeof( uint32_t ); // 4
            pos_data = pos + header_data.chunk_size;

            header_data.format = read_as_uint16( contents, pos );
            pos += sizeof( uint16_t ); // 2

            header_data.channels = read_as_uint16( contents, pos );
            pos += sizeof( uint16_t ); // 2

            header_data.sampes_per_sec = read_as_uint32( contents, pos );
            pos += sizeof( uint32_t ); // 4

            header_data.bytes_per_sec = read_as_uint32( contents, pos );
            pos += sizeof( uint32_t ); // 4

            header_data.block_align = read_as_uint16( contents, pos );
            pos += sizeof( uint16_t ); // 2

            header_data.bits_per_sample = read_as_uint16( contents, pos );
            pos += sizeof( uint16_t ); // 2

            break;
        }
    }

    if ( !found_fmt )
    {
        return false;
    }

    if ( pos != pos_data )
    {
        pos = pos_data;
    }

    bool found_data = false;

    while ( !found_data && pos + 4 < contents.size( ) )
    {
        read_as_chars( contents, pos, 4, header_data.data );
        pos += 4;

        if ( strncmp( header_data.data, LIST, strlen( LIST ) ) == 0 )
        {
            uint32_t info_size = read_as_uint32( contents, pos );
            pos += info_size;

            continue;
        }

        if ( strncmp( header_data.data, DATA, strlen( DATA ) ) == 0 )
        {
            found_data = true;

            header_data.data_size = read_as_uint32( contents, pos );
            pos += sizeof( uint32_t );

            break;
        }
    }

    if ( !found_data )
    {
        return false;
    }

    last_pos = pos;

    return true;
}

// -------------------------------------------------------------------------------------------------

bool
WAVEHelper::get_wave_data( const std::string& filename,
                           WAVEHeader& header_data,
                           int16_t*& left,
                           int16_t*& right )
{
    uint32_t pos = 0;

    if ( !get_wave_header( filename, header_data, pos ) )
    {
        return false;
    }

    uint32_t samples = header_data.data_size / header_data.block_align;
    std::ifstream input( filename, std::ifstream::in | std::ifstream::binary );

    if ( input.is_open( ) )
    {
        input.seekg( pos );
        left = new int16_t[ header_data.data_size / header_data.channels / sizeof( int16_t ) ];

        if ( header_data.channels == 1 )
        {
            input.read( ( char* )left, header_data.block_align * samples );
        }
        else
        {
            right = new int16_t[
                    header_data.data_size / header_data.channels / sizeof( int16_t ) ];

            for ( int i = 0; i < samples; i++ )
            {
                input.read( (char*)&left[ i ],
                            header_data.block_align / header_data.channels );

                input.read( (char*)&right[ i ],
                            header_data.block_align / header_data.channels );
            }
        }
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

} // utils
