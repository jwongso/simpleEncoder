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


#include <iostream>
#include <map>
#include <cstring>
#include <thread>

#include "core/EncoderMP3.h"
#include "utils/FileSystemHelper.h"

// -------------------------------------------------------------------------------------------------

const char*
error_to_string( common::ErrorCode error )
{
    static const std::map< common::ErrorCode, const char* > s_error_strings =
    {
        { common::ErrorCode::ERROR_NONE, "Error none" },
        { common::ErrorCode::ERROR_NOT_FOUND, "Not found" },
        { common::ErrorCode::ERROR_NOT_IMPLEMENTED, "Not implemented" },
        { common::ErrorCode::ERROR_PTHREAD_CREATE, "pthread create error" },
        { common::ErrorCode::ERROR_PTHREAD_JOIN, "pthread join error" },
        { common::ErrorCode::ERROR_BUSY, "pthread error" }
    };

    auto found = s_error_strings.find( error );

    if ( found == s_error_strings.end( ) )
    {
        return "Unknown error";
    }

    return found->second;
}

// -------------------------------------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    if ( argc < 2 )
    {
        std::cerr << "Please provide a valid path to directory of .WAV file(s)" << std::endl;
        std::cerr << "Usage: " << argv[ 0 ] << " <PATH DIRECTORY> [-jN]" << std::endl;

        return 0;
    }

    const std::string& folder = argv[ 1 ];
    std::string path;
    if ( !utils::FileSystemHelper::canonical_path( folder, path ) )
    {
        std::cerr << "The given directory: " << folder << " is not valid!" << std::endl;

        return 0;
    }

    uint16_t core_number = 1;
    uint16_t initial_core_numbers = std::thread::hardware_concurrency( );

    if ( initial_core_numbers > 2 )
    {
        core_number = initial_core_numbers / 2;
    }

    if ( argc > 2 )
    {
        if ( strncmp( argv[ 2 ], "-j", 2 ) == 0 )
        {
            char* num = &argv[ 2 ][ 2 ];

            int arg_num = atoi( num );
            if ( arg_num != 0 )
            {
                if ( arg_num <= initial_core_numbers )
                {
                    core_number = arg_num;
                }
            }
        }
    }

    core::EncoderMP3 encoder_mp3( common::AudioFormatType::WAV, core_number );

    auto error = encoder_mp3.scan_input_directory( path );

    if ( error != common::ErrorCode::ERROR_NONE )
    {
        std::cerr << "Error while scanning the input directory: " <<
                     error_to_string( error ) << std::endl;

        return 0;
    }

    const auto& wav_files = encoder_mp3.get_input_files( );

    if ( !wav_files.empty( ) )
    {
        std::cout << "Found " << wav_files.size( ) <<
                     " valid WAV files to be encoded using " <<
                     encoder_mp3.get_encoder_version( ) << ":" << std::endl;

        for ( const auto wav : wav_files )
        {
            std::cout << wav << std::endl;
        }

        error = encoder_mp3.start_encoding( );
    }

    return 0;
}

// -------------------------------------------------------------------------------------------------
