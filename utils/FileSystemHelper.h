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

#ifndef FILE_SYSTEM_HELPER_H
#define FILE_SYSTEM_HELPER_H

#include "IFileSystemHelper.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace utils
{

class FileSystemHelper : public IFileSystemHelper
{
public:
    /// Canonicalize a path from in with out as result.
    bool canonical_path(const std::string& in, std::string& out) override;

    /// Gets a value indicating whether the given file path exists.
    bool file_exists(const std::string& file_path) override;

    /// Gets a value indicating whether the given directory exists.
    bool directory_exists(const std::string& directory_path) override;

    /// Reads the contents of the given binary file into contents.
    bool read_binary_file(const std::string& file_path, std::vector<uint8_t>& contents) override;

    /// Reads the contents of the given binary file into contents.
    bool read_binary_file(const std::string& file_path, std::vector<int16_t>& contents) override;

    /// Retrieves file paths in the given directory recursively
    bool get_file_paths(const std::string& directory_path,
                        std::vector<std::string>& file_paths) override;

    /// Ensures a directory exists, creating it if necessary
    bool ensure_directory_exists(const std::string& dir) override;
};

}  // utils

#endif  // FILE_SYSTEM_HELPER_H
