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

#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <stdint.h>
#include <string>
#include <vector>

namespace utils
{
class FileSystemHelper
{
public:

    /// Canonicalize a path from in with out as result.
    static bool canonical_path( const std::string& in, std::string& out );

    /// Gets a value indicating whether the given file path exists.
    static bool file_exists( const std::string& file_path );

    /// Gets a value indicating whether the given directory exists.
    static bool directory_exists( const std::string& directory_path );

    /// Reads the contents of the given binary file into contents.
    static bool read_binary_file( const std::string& file_path, std::vector< uint8_t >& contents );

    /// Reads the contents of the given binary file into contents.
    static bool read_binary_file( const std::string& file_path, std::vector< int16_t >& contents );

    /// Retrieves file paths in the given directory recursively
    static bool get_file_paths( const std::string& directory_path,
                                std::vector< std::string >& file_paths );
};

}  // utils

#endif  // FILE_HELPER_H
