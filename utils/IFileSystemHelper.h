#ifndef I_FILE_SYSTEM_HELPER_H
#define I_FILE_SYSTEM_HELPER_H

#include <stdint.h>
#include <string>
#include <vector>

namespace utils
{

class IFileSystemHelper
{
public:
    virtual ~IFileSystemHelper() = default;

    /// Canonicalize a path from in with out as result.
    virtual bool canonical_path(const std::string& in, std::string& out) = 0;

    /// Gets a value indicating whether the given file path exists.
    virtual bool file_exists(const std::string& file_path) = 0;

    /// Gets a value indicating whether the given directory exists.
    virtual bool directory_exists(const std::string& directory_path) = 0;

    /// Reads the contents of the given binary file into contents.
    virtual bool read_binary_file(const std::string& file_path, std::vector<uint8_t>& contents) = 0;

    /// Reads the contents of the given binary file into contents.
    virtual bool read_binary_file(const std::string& file_path, std::vector<int16_t>& contents) = 0;

    /// Retrieves file paths in the given directory recursively
    virtual bool get_file_paths(const std::string& directory_path,
                               std::vector<std::string>& file_paths) = 0;

    /// Ensures a directory exists, creating it if necessary
    virtual bool ensure_directory_exists(const std::string& dir) = 0;
};

}  // namespace utils

#endif  // I_FILE_SYSTEM_HELPER_H
