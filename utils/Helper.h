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

#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>
#include <vector>
#include <string>
#include <functional>
#include <climits>

#define MP3_BIT 7

namespace utils
{

class Helper
{
public:

    static void read_as_chars( const std::vector< uint8_t >& input,
                               uint32_t pos,
                               uint32_t length,
                               char* target )
    {
        if ( sizeof( target ) / sizeof( char ) < length )
        {
            return;
        }

        std::copy( input.begin( ) + pos, input.begin( ) + ( pos + length ), target );
    }

    static uint32_t read_as_uint32_big( const std::vector< uint8_t >& input, uint32_t pos )
    {
        int res = 0;

        for ( uint32_t i = 0; i < sizeof( uint32_t ); i++ )
        {
            res = ( res << MP3_BIT ) + input[ pos + i ];
        }

        return res;
    }

    static uint32_t read_as_uint32_little( const std::vector< uint8_t >& input, uint32_t pos )
    {
        int res = 0;

        for ( uint32_t i = 0; i < sizeof( uint32_t ); i++ )
        {
            res += input[ pos + i ] << ( i * CHAR_BIT );
        }

        return res;
    }

    static uint16_t read_as_uint16( const std::vector< uint8_t >& input, uint32_t pos )
    {
        return ( input[ pos ] << 0 ) | ( input[ pos + 1 ] << 8 );
    }

    static std::string generate_output_file( const std::string& input_file,
                                             const std::string& extension )
    {
        std::string output;

        size_t pos = input_file.find_last_of( "." );

        if ( pos == std::string::npos )
        {
            // File has no extension. Just append the given extension.
            output = input_file + extension;
        }
        else
        {
            output = input_file.substr( 0, pos );
            output += extension;
        }

        return output;
    }

    static void log( const std::function<
                     void( const std::string&, const std::string& ) >& callback,
                     uint32_t id,
                     const std::string& status )
    {
#ifdef ENABLE_LOG
        callback( "oss.str( )", "Thread id: " + std::to_string( id ) + ", " + status );
#endif
    }
};

} // utils

#endif // HELPER_H
