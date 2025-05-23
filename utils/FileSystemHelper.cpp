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

#include "FileSystemHelper.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <sstream>
#include <tuple>
#include <filesystem>

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

namespace utils
{

namespace
{

#define UNUSED( var ) do { ( void )( var ); } while ( 0 )

// Directory separator.
static const char separator = '/';

template < class Fun >
std::tuple< bool, bool >
iterate_directory( const std::string& path, bool recursive, bool skip_directories, Fun fun )
{
    // Attempt to open the directory.
    DIR* dp = opendir( path.c_str( ) );
    if ( !dp )
    {
        return std::make_tuple( false, false );
    }

    // Iterate the directory contents.
    bool status = true;
    bool stop = false;
    struct dirent* entry;
    while ( ( ( entry = readdir( dp ) ) != nullptr ) && ( status ) && ( !stop ) )
    {
        // Skip the special "." and ".." entries.
        std::string entry_name( entry->d_name );
        if ( ( entry_name != "." ) && ( entry_name != ".." ) )
        {
            // Get information about the current entry.
            std::string entry_path = path + separator + entry_name;
            struct stat stat_buf;
            if ( lstat( entry_path.c_str( ), &stat_buf ) == 0 )
            {
                // If this is a directory, see if we should recurse into it.
                if ( ( recursive ) && ( S_ISDIR( stat_buf.st_mode ) ) )
                {
                    std::tie( status, stop )
                        = iterate_directory( entry_path, true, skip_directories, fun );
                }

                // Check if any errors were encountered or if the iteration is to be stopped.
                if ( status && !stop )
                {
                    // Continue if we don't want to have folder names in the output.
                    if ( S_ISDIR( stat_buf.st_mode ) && skip_directories )
                    {
                        continue;
                    }

                    // Call the passed in function on the current entry.
                    stop = fun( entry_path, entry_name, stat_buf );
                }
            }
            else
            {
                status = false;
            }
        }
    }

    closedir( dp );

    return std::make_tuple( status, stop );
}
}

// ------------------------------------------------------------------------------------------------

bool
FileSystemHelper::canonical_path( const std::string& in, std::string& out )
{
    const char* cstr_in = in.c_str( );
    char cstr_out[ PATH_MAX ];
    char* out_ptr;

    out_ptr = realpath( cstr_in, cstr_out );

    if ( !out_ptr )
    {
        out.assign( cstr_in );

        return false;
    }

    out.assign( out_ptr );

    return true;
}

// -------------------------------------------------------------------------------------------------

bool
FileSystemHelper::file_exists( const std::string& file_path )
{
    if ( file_path.empty( ) )
    {
        return false;
    }

    struct stat stat_info;

    return ( 0 == stat( file_path.c_str( ), &stat_info ) ) && S_ISREG( stat_info.st_mode );
}

// -------------------------------------------------------------------------------------------------

bool
FileSystemHelper::directory_exists( const std::string& directory_path )
{
    if ( directory_path.empty( ) )
    {
        return false;
    }

    struct stat stat_info;

    return ( 0 == stat( directory_path.c_str( ), &stat_info ) ) && S_ISDIR( stat_info.st_mode );
}

// -------------------------------------------------------------------------------------------------

bool
FileSystemHelper::read_binary_file( const std::string& file_path, std::vector< uint8_t >& contents )
{
    if ( !FileSystemHelper::file_exists( file_path ) )
    {
        return false;
    }

    FILE* fp;

    fp = ::fopen( file_path.c_str( ), "rb" );

    if ( !fp )
    {
        return false;
    }

    ::fseek( fp, 0, SEEK_END );
    const auto file_size = ::ftell( fp );
    ::fseek( fp, 0, SEEK_SET );

    contents.clear( );
    contents.resize(file_size / sizeof(uint8_t));

    size_t readItems = ::fread( &contents[ 0 ], file_size, 1, fp );
    ::fclose( fp );

    return readItems == 1;
}

// -------------------------------------------------------------------------------------------------

bool
FileSystemHelper::read_binary_file( const std::string& file_path, std::vector< int16_t >& contents )
{
    if ( !FileSystemHelper::file_exists( file_path ) )
    {
        return false;
    }

    FILE* fp;

    fp = ::fopen( file_path.c_str( ), "rb" );

    if ( !fp )
    {
        return false;
    }

    ::fseek( fp, 0, SEEK_END );
    const auto file_size = ::ftell( fp );
    ::fseek( fp, 0, SEEK_SET );

    contents.clear( );
    contents.resize(file_size / sizeof(int16_t));

    size_t readItems = ::fread( ( char* )&contents[ 0 ], file_size, 1, fp );
    ::fclose( fp );

    return readItems == 1;
}

// -------------------------------------------------------------------------------------------------

bool
FileSystemHelper::get_file_paths( const std::string& directory_path,
                            std::vector< std::string >& file_paths )
{
    // Iterate the directory contents recursively.
    bool status;
    std::tie( status, std::ignore )
        = iterate_directory( directory_path, true, false,
                             [&]( const std::string& entry_path, const std::string& entry_name,
                                  const struct stat& sinfo ) -> bool
                             {
                                 UNUSED( entry_name );
                                 UNUSED( sinfo );

                                 file_paths.push_back( entry_path );
                                 return false;
                             } );

    return status;
}

// -------------------------------------------------------------------------------------------------

bool
FileSystemHelper::ensure_directory_exists( const std::string& dir )
{
    namespace fs = std::filesystem;

    try {
        fs::path path(dir);

        // If the directory already exists, return true
        if (fs::exists(path)) {
            return fs::is_directory(path);
        }

        // Try to create the directory (and any necessary parent dirs)
        return fs::create_directories(path);
    } catch (const fs::filesystem_error& e) {
        // Optionally log the error
        // std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }
}

// -------------------------------------------------------------------------------------------------

}  // utils
