#ifndef MOCK_FILE_SYSTEM_HELPER_H
#define MOCK_FILE_SYSTEM_HELPER_H

#include "utils/IFileSystemHelper.h"
#include <gmock/gmock.h>

namespace utils
{
namespace testing
{

class MockFileSystemHelper : public IFileSystemHelper
{
public:
    MOCK_METHOD(bool, canonical_path, (const std::string& in, std::string& out), (override));
    MOCK_METHOD(bool, file_exists, (const std::string& file_path), (override));
    MOCK_METHOD(bool, directory_exists, (const std::string& directory_path), (override));
    MOCK_METHOD(bool, read_binary_file, (const std::string& file_path, std::vector<uint8_t>& contents), (override));
    MOCK_METHOD(bool, read_binary_file, (const std::string& file_path, std::vector<int16_t>& contents), (override));
    MOCK_METHOD(bool, get_file_paths, (const std::string& directory_path, std::vector<std::string>& file_paths), (override));
    MOCK_METHOD(bool, ensure_directory_exists, (const std::string& dir), (override));
};

}  // namespace testing
}  // namespace utils

#endif  // MOCK_FILE_SYSTEM_HELPER_H
