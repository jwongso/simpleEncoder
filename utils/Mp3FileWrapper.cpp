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

#include "Mp3FileWrapper.h"
#include "FileSystemHelper.h"

#include <cstring>
#include <fstream>
#include <regex>

namespace utils
{

namespace
{

const char* ID3                         = "ID3";

#define ID3_FLAG_FOOTER_PRESENT         0
#define ID3_FLAG_EXPERIMENTAL_INDICATOR 1
#define ID3_FLAG_EXTENDED_HEADER        2
#define ID3_FLAG_UNSYNCHRONISATION      3

// -------------------------------------------------------------------------------------------------

template< class T >
void
read_as_chars( const std::vector< T >& input, uint32_t pos, uint32_t length, char* target )
{
    if ( sizeof( target ) / sizeof( char ) < length )
    {
        return;
    }

    std::copy( input.begin( ) + pos, input.begin( ) + ( pos + length ), target );
}

template< class T >
uint32_t
read_as_uint32( const std::vector< T >& input, uint32_t pos )
{
    return ( input[ pos ] |
           ( input[ pos + 1 ] << 8 ) |
           ( input[ pos + 2 ] << 16 ) |
           ( input[ pos + 3 ] << 24 ) );
}

template< class T >
uint16_t
read_as_uint16( const std::vector< T >& input, uint32_t pos )
{
    return ( input[ pos ] << 0 ) | ( input[ pos + 1 ] << 8 );
}

bool
get_flags( const uint8_t flag, ID3Tag& tag )
{
    for ( int i = 0; i < 4; i++ )
    {
        if ( flag >> i & 1 )
        {
            return false;
        }
    }

    for ( int i = 4; i < 8; i++ )
    {
        if ( flag >> i & 1 )
        {
            tag.flags[ i - 4 ] = true;
        }
        else
        {
            tag.flags[ i - 4 ] = false;
        }
    }

    return true;
}

}

// -------------------------------------------------------------------------------------------------

Mp3FileWrapper::Mp3FileWrapper( const std::string& filename )
    : m_filename( filename )
{
    m_valid = validate( m_filename, m_id3tags, m_header );
}

// -------------------------------------------------------------------------------------------------

bool
Mp3FileWrapper::is_valid( ) const
{
    return m_valid;
}

// -------------------------------------------------------------------------------------------------
/*
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
*/
// -------------------------------------------------------------------------------------------------

bool
Mp3FileWrapper::validate( const std::string& filename,
                          std::vector< ID3Tag >& id3tags,
                          Mp3Header& header )
{
    std::vector< uint8_t > contents;

    if ( !FileSystemHelper::read_binary_file( filename, contents ) )
    {
        return false;
    }

    uint32_t pos = 0;
    get_id3tags( contents, id3tags, pos );

}

// -------------------------------------------------------------------------------------------------

bool
Mp3FileWrapper::get_id3tags( const std::vector< uint8_t >& contents,
                             std::vector< ID3Tag >& id3tags,
                             uint32_t& last_pos )
{
    uint32_t pos = 0;
    bool id3_exists = true;

    while ( id3_exists )
    {
        ID3Tag id3tag;
        read_as_chars( contents, pos, 3, id3tag.id3 );

        if ( strncmp( id3tag.id3, ID3, 3 ) != 0 )
        {
            id3_exists = false;

            break;
        }

        pos += sizeof( id3tag.id3 );
        read_as_chars( contents, pos, 2, id3tag.version );
        pos += sizeof( id3tag.version );

        if ( get_flags( contents[ pos ], id3tag ) )
        {
            pos += sizeof( uint8_t );
            id3tag.offset = read_as_uint32( contents, pos );
            pos += sizeof( id3tag.offset );

            uint32_t size = read_as_uint32( contents, pos );

            if ( id3tag.flags[ ID3_FLAG_EXTENDED_HEADER ] )
            {
                id3tag.extended_size = size;
            }
            else
            {
                id3tag.extended_size = 0;
            }

            pos += id3tag.extended_size;

            int footer_size = id3tag.flags[ ID3_FLAG_FOOTER_PRESENT ] * 10;
            int frame_size = id3tag.offset - id3tag.extended_size - footer_size;

            std::regex reg( "[A-Z0-9]" );

            while( !std::regex_match( ( std::string ) { ( char )contents[ pos ] }, reg ) &&
                   pos < frame_size )
            {
                std::string id;
                std::string content;
                int field_size = 0;

                for ( int i = pos; i < pos + 4; i++ )
                {
                    id += contents[ i ];
                }

                id3tag.frames[ 0 ].push_back( id );

                pos += 4;
                field_size = read_as_uint32( contents, pos );

                pos += 6;

                for ( int i = pos; i < pos + field_size; i++ )
                {
                    content += contents[ i ];
                }

                id3tag.frames[ 1 ].push_back( content );

                pos += field_size;
            }
        }

        id3tags.push_back( id3tag );
        pos += id3tag.offset + 10;
    }

    last_pos = pos;
}

// -------------------------------------------------------------------------------------------------

} // utils
