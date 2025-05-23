#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "utils/FileSystemHelper.h"
#include <fstream>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

class FileSystemHelperTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a temporary test directory
        test_dir = fs::temp_directory_path() / "file_system_helper_test";
        fs::create_directories(test_dir);

        // Create a test file
        test_file = test_dir / "test_file.txt";
        std::ofstream file(test_file);
        file << "Test content";
        file.close();

        // Create a binary test file
        binary_test_file = test_dir / "binary_test.bin";
        std::ofstream binary_file(binary_test_file, std::ios::binary);
        std::vector<uint8_t> binary_data = {0x01, 0x02, 0x03, 0x04};
        binary_file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
        binary_file.close();

        // Create a nested directory
        nested_dir = test_dir / "nested";
        fs::create_directories(nested_dir);

        // Create a file in the nested directory
        nested_file = nested_dir / "nested_file.txt";
        std::ofstream nested(nested_file);
        nested << "Nested content";
        nested.close();
    }

    void TearDown() override
    {
        // Clean up test directory and all contents
        fs::remove_all(test_dir);
    }

    fs::path test_dir;
    fs::path test_file;
    fs::path binary_test_file;
    fs::path nested_dir;
    fs::path nested_file;
    utils::FileSystemHelper fs_helper;
};

TEST_F(FileSystemHelperTest, CanonicalPath)
{
    std::string result;
    EXPECT_TRUE(fs_helper.canonical_path(test_file.string(), result));
    EXPECT_EQ(fs::canonical(test_file).string(), result);

    // Test with non-existent path
    std::string non_existent = (test_dir / "non_existent.txt").string();
    EXPECT_FALSE(fs_helper.canonical_path(non_existent, result));
}

TEST_F(FileSystemHelperTest, FileExists)
{
    EXPECT_TRUE(fs_helper.file_exists(test_file.string()));
    EXPECT_FALSE(fs_helper.file_exists((test_dir / "non_existent.txt").string()));
    EXPECT_FALSE(fs_helper.file_exists(test_dir.string())); // Directory, not a file
    EXPECT_FALSE(fs_helper.file_exists("")); // Empty string
}

TEST_F(FileSystemHelperTest, DirectoryExists)
{
    EXPECT_TRUE(fs_helper.directory_exists(test_dir.string()));
    EXPECT_TRUE(fs_helper.directory_exists(nested_dir.string()));
    EXPECT_FALSE(fs_helper.directory_exists(test_file.string())); // File, not a directory
    EXPECT_FALSE(fs_helper.directory_exists((test_dir / "non_existent").string()));
    EXPECT_FALSE(fs_helper.directory_exists("")); // Empty string
}

TEST_F(FileSystemHelperTest, ReadBinaryFileUint8)
{
    std::vector<uint8_t> contents;
    EXPECT_TRUE(fs_helper.read_binary_file(binary_test_file.string(), contents));
    EXPECT_EQ(4, contents.size());
    EXPECT_EQ(0x01, contents[0]);
    EXPECT_EQ(0x02, contents[1]);
    EXPECT_EQ(0x03, contents[2]);
    EXPECT_EQ(0x04, contents[3]);

    // Test with non-existent file
    EXPECT_FALSE(fs_helper.read_binary_file((test_dir / "non_existent.bin").string(), contents));
}

TEST_F(FileSystemHelperTest, ReadBinaryFileInt16)
{
    // Create a specific int16 test file
    fs::path int16_test_file = test_dir / "int16_test.bin";
    std::ofstream int16_file(int16_test_file, std::ios::binary);
    std::vector<int16_t> int16_data = {0x0101, 0x0202};
    int16_file.write(reinterpret_cast<const char*>(int16_data.data()), int16_data.size() * sizeof(int16_t));
    int16_file.close();

    std::vector<int16_t> contents;
    EXPECT_TRUE(fs_helper.read_binary_file(int16_test_file.string(), contents));
    EXPECT_EQ(2, contents.size());
    EXPECT_EQ(0x0101, contents[0]);
    EXPECT_EQ(0x0202, contents[1]);

    // Test with non-existent file
    EXPECT_FALSE(fs_helper.read_binary_file((test_dir / "non_existent.bin").string(), contents));
}

TEST_F(FileSystemHelperTest, GetFilePaths)
{
    std::vector<std::string> file_paths;
    EXPECT_TRUE(fs_helper.get_file_paths(test_dir.string(), file_paths));

    // Should find at least 3 files (test_file, binary_test_file, nested_file)
    EXPECT_GE(file_paths.size(), 3);

    // Check if all our test files are in the result
    auto contains_file = [&file_paths](const std::string& path) {
        return std::find(file_paths.begin(), file_paths.end(), path) != file_paths.end();
    };

    EXPECT_TRUE(contains_file(test_file.string()));
    EXPECT_TRUE(contains_file(binary_test_file.string()));
    EXPECT_TRUE(contains_file(nested_file.string()));

    // Test with non-existent directory
    file_paths.clear();
    EXPECT_FALSE(fs_helper.get_file_paths((test_dir / "non_existent").string(), file_paths));
    EXPECT_TRUE(file_paths.empty());
}

TEST_F(FileSystemHelperTest, EnsureDirectoryExists)
{
    // Test with existing directory
    EXPECT_TRUE(fs_helper.ensure_directory_exists(test_dir.string()));

    // Test creating a new directory
    fs::path new_dir = test_dir / "new_directory";
    EXPECT_TRUE(fs_helper.ensure_directory_exists(new_dir.string()));
    EXPECT_TRUE(fs::exists(new_dir));
    EXPECT_TRUE(fs::is_directory(new_dir));

    // Test with nested new directories
    fs::path nested_new_dir = test_dir / "a" / "b" / "c";
    EXPECT_TRUE(fs_helper.ensure_directory_exists(nested_new_dir.string()));
    EXPECT_TRUE(fs::exists(nested_new_dir));
    EXPECT_TRUE(fs::is_directory(nested_new_dir));

    // Test with a file path (should fail)
    EXPECT_FALSE(fs_helper.ensure_directory_exists(test_file.string()));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
