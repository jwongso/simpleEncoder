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
#include "Helper.h"

#include <cstring>
#include <fstream>
#include <regex>

namespace utils
{

namespace
{

const char* ID3                         = "ID3";
const char* COMM                        = "COMM";

#define ID3_FRAME_ID_LENGTH             4
#define ID3_FLAG_FOOTER_PRESENT         0
#define ID3_FLAG_EXPERIMENTAL_INDICATOR 1
#define ID3_FLAG_EXTENDED_HEADER        2
#define ID3_FLAG_UNSYNCHRONISATION      3

// -------------------------------------------------------------------------------------------------

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

void
get_mpeg_version_layer_crc( const uint8_t val, Mp3Header& header )
{
    if ( ( val & 0x10 ) == 0x10 && ( val & 0x08 ) == 0x08 )
    {
        header.mpeg_version = 1;
    }
    else if ( ( val & 0x10) == 0x10 && ( val & 0x08 ) != 0x08 )
    {
        header.mpeg_version = 2;
    }
    else if ( ( val & 0x10 ) != 0x10 && ( val & 0x08 ) == 0x08 )
    {
        header.mpeg_version = 0;
    }
    else if ( ( val & 0x10 ) != 0x10 && ( val & 0x08 ) != 0x08 )
    {
        header.mpeg_version = 2.5;
    }
    else
    {
        header.mpeg_version = 0;
    }

    uint8_t layer = val << 5;
    layer = val >> 6;
    header.layer = 4 - layer;

    header.crc = val & 0x01;
}

void
get_sampling_rate( const uint8_t val, Mp3Header& header )
{
    const int rates[ 3 ][ 3 ] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

    for ( int version = 1; version <= 3; version++ )
    {
        if ( header.mpeg_version == version )
        {
            if ( ( val & 0x08 ) != 0x08 && ( val & 0x04 ) != 0x04 )
            {
                header.sampling_rate = rates[ version - 1 ][ 0 ];

                break;
            }
            else if ( ( val & 0x08 ) != 0x08 && ( val & 0x04 ) == 0x04 )
            {
                header.sampling_rate = rates[ version - 1 ][ 1 ];

                break;
            }
            else if ( ( val & 0x08 ) == 0x08 && ( val & 0x04 ) != 0x04 )
            {
                header.sampling_rate = rates[ version - 1 ][ 2 ];

                break;
            }
        }
    }
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
    utils::FileSystemHelper fs;
    std::vector< uint8_t > contents;

    if ( !fs.read_binary_file( filename, contents ) )
    {
        return false;
    }

    uint32_t pos = 0;
    get_id3tags( contents, id3tags, pos );
    return get_mp3header( contents, pos, header );
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
        Helper::read_as_chars( contents, pos, 3, id3tag.id3 );

        if ( strncmp( id3tag.id3, ID3, 3 ) != 0 )
        {
            id3_exists = false;

            break;
        }

        pos += sizeof( id3tag.id3 );

        id3tag.version_major = contents[ pos ];
        pos += sizeof( id3tag.version_major );
        id3tag.version_revision = contents[ pos ];
        pos += sizeof( id3tag.version_major );

        bool id3v2 = false;
        // Check whether id3v2 or v1
        if ( get_flags( contents[ pos ], id3tag ) )
        {
            id3v2 = true;
            pos += sizeof( uint8_t );
            id3tag.offset = Helper::read_as_uint32_big( contents, pos );
            pos += sizeof( id3tag.offset );

            if ( id3tag.flags[ ID3_FLAG_EXTENDED_HEADER ] == true )
            {
                uint32_t size = Helper::read_as_uint32_big( contents, pos );
                id3tag.extended_size = size;
                pos += id3tag.extended_size;
            }
            else
            {
                id3tag.extended_size = 0;
            }
        }

        int footer_size = id3tag.flags[ ID3_FLAG_FOOTER_PRESENT ] * 10;
        int frame_size = id3tag.offset - id3tag.extended_size - footer_size;

        std::regex reg( "[A-Z0-9]" );

        while( std::regex_match( ( std::string ) { ( char )contents[ pos ] }, reg ) &&
               pos < frame_size )
        {
            std::string key;
            std::string value;
            int field_size = 0;

            for ( int i = pos; i < pos + ID3_FRAME_ID_LENGTH; i++ )
            {
                key += contents[ i ];
            }

            pos += ID3_FRAME_ID_LENGTH;
            field_size = Helper::read_as_uint32_big( contents, pos );

            // Skipping the flags bytes (2 bytes) + 4 bytes of previous field size
            pos += 6;

            for ( int i = pos; i < pos + field_size; i++ )
            {
                if ( key[ 0 ] == 'T' )
                {
                    if ( id3v2 )
                    {
                        // We skip the 1. byte since it's only about encoding:
                        // 00 = Latin ISO 8859-1 and 01 = Unicode
                        if ( i != pos )
                        {
                            value += contents[ i ];
                        }
                    }
                    else
                    {
                        value += contents[ i ];
                    }
                }
                else if ( key == COMM )
                {
                    if ( id3v2 )
                    {
                        // We skip the first 4 bytes for Comments frame because:
                        // 1. byte is text encoding, and the next 3 bytes are used for language
                        if ( i > pos + 4 )
                        {
                            value += contents[ i ];
                        }
                    }
                    else
                    {
                        value += contents[ i ];
                    }
                }
            }

            id3tag.frames[ key ] = value;

            pos += field_size;
        }

        id3tags.push_back( id3tag );
        pos = id3tag.offset + 10;
    }

    last_pos = pos;

    return id3_exists;
}

// -------------------------------------------------------------------------------------------------

bool
Mp3FileWrapper::get_mp3header( const std::vector< uint8_t >& contents,
                               const uint32_t offset,
                               Mp3Header& header )
{
    uint32_t pos = offset;

    if ( pos >= contents.size( ) )
    {
        return false;
    }

    if ( contents[ pos ] != 0xFF ) // 0
    {
        return false;
    }

    get_mpeg_version_layer_crc( contents[ pos + 1 ], header ); // 1

    header.info[ 0 ] = contents[ pos + 2 ] & 0x01; // 2
    header.info[ 1 ] = contents[ pos + 3 ] & 0x08; // 3
    header.info[ 2 ] = contents[ pos + 4 ] & 0x04; // 4

    header.emphasis = contents[ pos + 3 ] << 6; // 3
    header.emphasis = header.emphasis >> 6;

    get_sampling_rate( contents[ pos + 2 ], header ); // 2

    return true;
}

// -------------------------------------------------------------------------------------------------

} // utils
