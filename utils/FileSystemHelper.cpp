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

#ifdef _MSC_VER

#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <codecvt>
#include <Strsafe.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

// For Windows there is no equivalent for lstat.
#define stat _stat
#define lstat _stat

// Convenience macros for stat mode not present in Windows.
#define S_ISDIR( mode ) ( ( mode & S_IFMT ) == S_IFDIR )
#define S_ISCHR( mode ) ( ( mode & S_IFMT ) == S_IFCHR )
#define S_ISFIFO( mode ) ( ( mode & S_IFMT ) == S_IFFIFO )
#define S_ISREG( mode ) ( ( mode & S_IFMT ) == S_IFREG )

#else  // _MSC_VER

#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

#endif  // _MSC_VER

namespace utils
{

namespace
{

#define UNUSED( var ) do { ( void )( var ); } while ( 0 )

#if defined( _MSC_VER )

// Directory separator.
static const char separator = '\\';

struct dirent
{
    char* d_name;
};

struct DIR
{
    intptr_t handle;
    struct _finddata_t info;
    dirent result;
    char* name;
};

DIR*
opendir( const char* name )
{
    // Check for valid directory path.
    if ( ( name == nullptr ) || ( name[ 0 ] == 0 ) )
    {
        errno = EINVAL;
        return nullptr;
    }

    // Determine the trailing search pattern string.
    std::size_t base_len = strlen( name );
    const char* pat = ( strchr( "/\\", name[ base_len - 1 ] ) ) ? "*" : "\\*";
    std::size_t size = base_len + strlen( pat ) + 1;

    // Allocate the DIR struct along with its name.
    DIR* dir = static_cast< DIR* >( malloc( sizeof *dir ) );
    if ( !dir || ( ( dir->name = static_cast< char* >( malloc( size ) ) ) == nullptr ) )
    {
        free( dir );
        errno = ENOMEM;
        return nullptr;
    }

    // Set up the directory traversal information.
    strcat( strcpy( dir->name, name ), pat );
    dir->result.d_name = nullptr;
    if ( ( dir->handle = _findfirst( dir->name, &dir->info ) ) == -1 )
    {
        free( dir->name );
        free( dir );
        dir = nullptr;
    }

    return dir;
}

int
closedir( DIR* dir )
{
    if ( dir == nullptr )
    {
        errno = EBADF;
        return -1;
    }

    int result = _findclose( dir->handle );
    if ( result < 0 )
    {
        errno = EBADF;
    }

    free( dir->name );
    free( dir );

    return result;
}

struct dirent*
readdir( DIR* dir )
{
    if ( ( dir == nullptr ) || ( dir->handle == -1 ) )
    {
        errno = EBADF;
        return nullptr;
    }

    struct dirent* result = nullptr;
    if ( ( dir->result.d_name == nullptr ) || ( _findnext( dir->handle, &dir->info ) != -1 ) )
    {
        result = &dir->result;
        result->d_name = dir->info.name;
    }

    return result;
}

#else

// Directory separator.
static const char separator = '/';

#endif

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

#if defined( _MSC_VER )
    char cstr_out[ MAX_PATH ];

    DWORD rv = GetFullPathNameA( cstr_in, MAX_PATH, cstr_out, nullptr );

    if ( rv == 0 )
    {
        out.assign( in );

        return false;
    }

    out.assign( cstr_out );
#else
    char cstr_out[ PATH_MAX ];
    char* out_ptr;

    out_ptr = realpath( cstr_in, cstr_out );

    if ( !out_ptr )
    {
        out.assign( cstr_in );

        return false;
    }

    out.assign( out_ptr );
#endif

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

#ifdef _MSC_VER

    errno_t err = fopen_s( &fp, file_path.c_str( ), "rb" );

    if ( err != 0 )
    {
        return false;
    }

#else

    fp = ::fopen( file_path.c_str( ), "rb" );

    if ( !fp )
    {
        return false;
    }

#endif  // _MSC_VER

    ::fseek( fp, 0, SEEK_END );
    const auto file_size = ::ftell( fp );
    ::fseek( fp, 0, SEEK_SET );

    contents.clear( );
    contents.resize( file_size );

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

#ifdef _MSC_VER

    errno_t err = fopen_s( &fp, file_path.c_str( ), "rb" );

    if ( err != 0 )
    {
        return false;
    }

#else

    fp = ::fopen( file_path.c_str( ), "rb" );

    if ( !fp )
    {
        return false;
    }

#endif  // _MSC_VER

    ::fseek( fp, 0, SEEK_END );
    const auto file_size = ::ftell( fp );
    ::fseek( fp, 0, SEEK_SET );

    contents.clear( );
    contents.resize( file_size );

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
