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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

#include "tests/AudioValidationHelper.h"
#include "utils/FileSystemHelper.h"

namespace fs = std::filesystem;

/**
 * @class AudioValidationTest
 * @brief Test suite for the AudioValidationHelper utility.
 *
 * This test class validates the AudioValidationHelper's ability to:
 * - Detect audio file formats correctly
 * - Validate file integrity
 * - Generate comprehensive validation reports
 * - Compare encoded files with originals
 */
class AudioValidationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Set up test directories
        test_output_dir = fs::temp_directory_path() / "audio_validation_test";
        
        // Ensure output directory exists and is clean
        if (fs::exists(test_output_dir)) {
            fs::remove_all(test_output_dir);
        }
        fs::create_directories(test_output_dir);
        
        // Create sample test files for validation
        createTestFiles();
    }

    void TearDown() override
    {
        // Clean up test output directory
        if (fs::exists(test_output_dir)) {
            fs::remove_all(test_output_dir);
        }
    }

    /**
     * @brief Creates sample test files with different audio format signatures.
     */
    void createTestFiles()
    {
        // Create a fake MP3 file with ID3 header
        createFakeMP3File();
        
        // Create a fake FLAC file with FLAC signature
        createFakeFLACFile();
        
        // Create a fake AAC file with ADTS header
        createFakeAACFile();
        
        // Create a fake OGG Vorbis file
        createFakeVorbisFile();
        
        // Create a fake OGG Opus file
        createFakeOpusFile();
        
        // Create an invalid file
        createInvalidFile();
        
        // Create an empty file
        createEmptyFile();
    }

    void createFakeMP3File()
    {
        fs::path mp3_file = test_output_dir / "test.mp3";
        std::ofstream file(mp3_file, std::ios::binary);
        
        // Write ID3v2 header
        file.write("ID3", 3);
        file.write("\x03\x00", 2); // Version 2.3
        file.write("\x00", 1);     // Flags
        file.write("\x00\x00\x00\x00", 4); // Size (0 for simplicity)
        
        // Write some fake MP3 frame data
        uint8_t mp3_frame[] = {0xFF, 0xFB, 0x90, 0x00}; // MP3 frame sync
        file.write(reinterpret_cast<const char*>(mp3_frame), sizeof(mp3_frame));
        
        // Add some dummy data
        std::vector<uint8_t> dummy_data(1024, 0xAA);
        file.write(reinterpret_cast<const char*>(dummy_data.data()), dummy_data.size());
    }

    void createFakeFLACFile()
    {
        fs::path flac_file = test_output_dir / "test.flac";
        std::ofstream file(flac_file, std::ios::binary);
        
        // Write FLAC signature
        file.write("fLaC", 4);
        
        // Add some dummy metadata and data
        std::vector<uint8_t> dummy_data(1024, 0xBB);
        file.write(reinterpret_cast<const char*>(dummy_data.data()), dummy_data.size());
    }

    void createFakeAACFile()
    {
        fs::path aac_file = test_output_dir / "test.aac";
        std::ofstream file(aac_file, std::ios::binary);
        
        // Write ADTS sync word
        uint8_t adts_header[] = {0xFF, 0xF1}; // ADTS sync
        file.write(reinterpret_cast<const char*>(adts_header), sizeof(adts_header));
        
        // Add some dummy data
        std::vector<uint8_t> dummy_data(1024, 0xCC);
        file.write(reinterpret_cast<const char*>(dummy_data.data()), dummy_data.size());
    }

    void createFakeVorbisFile()
    {
        fs::path vorbis_file = test_output_dir / "test.ogg";
        std::ofstream file(vorbis_file, std::ios::binary);
        
        // Write OGG header
        file.write("OggS", 4);
        file.write("\x00", 1); // Version
        file.write("\x02", 1); // Header type (first page)
        
        // Skip to where we'd put the vorbis identification
        std::vector<uint8_t> padding(20, 0x00);
        file.write(reinterpret_cast<const char*>(padding.data()), padding.size());
        
        // Write vorbis identification
        file.write("\x01vorbis", 7);
        
        // Add some dummy data
        std::vector<uint8_t> dummy_data(1024, 0xDD);
        file.write(reinterpret_cast<const char*>(dummy_data.data()), dummy_data.size());
    }

    void createFakeOpusFile()
    {
        fs::path opus_file = test_output_dir / "test.opus";
        std::ofstream file(opus_file, std::ios::binary);
        
        // Write OGG header
        file.write("OggS", 4);
        file.write("\x00", 1); // Version
        file.write("\x02", 1); // Header type (first page)
        
        // Skip to where we'd put the opus identification
        std::vector<uint8_t> padding(20, 0x00);
        file.write(reinterpret_cast<const char*>(padding.data()), padding.size());
        
        // Write OpusHead identification
        file.write("OpusHead", 8);
        
        // Add some dummy data
        std::vector<uint8_t> dummy_data(1024, 0xEE);
        file.write(reinterpret_cast<const char*>(dummy_data.data()), dummy_data.size());
    }

    void createInvalidFile()
    {
        fs::path invalid_file = test_output_dir / "invalid.mp3";
        std::ofstream file(invalid_file, std::ios::binary);
        
        // Write some random data that doesn't match any audio format
        std::vector<uint8_t> random_data = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
        file.write(reinterpret_cast<const char*>(random_data.data()), random_data.size());
    }

    void createEmptyFile()
    {
        fs::path empty_file = test_output_dir / "empty.mp3";
        std::ofstream file(empty_file, std::ios::binary);
        // File is created but left empty
    }

protected:
    fs::path test_output_dir;
};

// Test individual file validation
TEST_F(AudioValidationTest, ValidateMP3File)
{
    fs::path mp3_file = test_output_dir / "test.mp3";
    auto result = AudioValidationHelper::validateAudioFile(mp3_file.string());
    
    EXPECT_TRUE(result.is_valid) << "MP3 file should be valid";
    EXPECT_EQ(result.format, AudioValidationHelper::AudioFormat::MP3);
    EXPECT_GT(result.file_size, 0) << "File size should be greater than 0";
    EXPECT_FALSE(result.metadata["format_details"].empty()) << "Format details should be present";
}

TEST_F(AudioValidationTest, ValidateFLACFile)
{
    fs::path flac_file = test_output_dir / "test.flac";
    auto result = AudioValidationHelper::validateAudioFile(flac_file.string());
    
    EXPECT_TRUE(result.is_valid) << "FLAC file should be valid";
    EXPECT_EQ(result.format, AudioValidationHelper::AudioFormat::FLAC);
    EXPECT_GT(result.file_size, 0) << "File size should be greater than 0";
}

TEST_F(AudioValidationTest, ValidateAACFile)
{
    fs::path aac_file = test_output_dir / "test.aac";
    auto result = AudioValidationHelper::validateAudioFile(aac_file.string());
    
    EXPECT_TRUE(result.is_valid) << "AAC file should be valid";
    EXPECT_EQ(result.format, AudioValidationHelper::AudioFormat::AAC);
    EXPECT_GT(result.file_size, 0) << "File size should be greater than 0";
}

TEST_F(AudioValidationTest, ValidateVorbisFile)
{
    fs::path vorbis_file = test_output_dir / "test.ogg";
    auto result = AudioValidationHelper::validateAudioFile(vorbis_file.string());
    
    EXPECT_TRUE(result.is_valid) << "Vorbis file should be valid";
    EXPECT_EQ(result.format, AudioValidationHelper::AudioFormat::VORBIS);
    EXPECT_GT(result.file_size, 0) << "File size should be greater than 0";
}

TEST_F(AudioValidationTest, ValidateOpusFile)
{
    fs::path opus_file = test_output_dir / "test.opus";
    auto result = AudioValidationHelper::validateAudioFile(opus_file.string());
    
    EXPECT_TRUE(result.is_valid) << "Opus file should be valid";
    EXPECT_EQ(result.format, AudioValidationHelper::AudioFormat::OPUS);
    EXPECT_GT(result.file_size, 0) << "File size should be greater than 0";
}

// Test invalid file handling
TEST_F(AudioValidationTest, ValidateInvalidFile)
{
    fs::path invalid_file = test_output_dir / "invalid.mp3";
    auto result = AudioValidationHelper::validateAudioFile(invalid_file.string());
    
    EXPECT_FALSE(result.is_valid) << "Invalid file should not be valid";
    EXPECT_FALSE(result.error_message.empty()) << "Error message should be present";
}

TEST_F(AudioValidationTest, ValidateEmptyFile)
{
    fs::path empty_file = test_output_dir / "empty.mp3";
    auto result = AudioValidationHelper::validateAudioFile(empty_file.string());
    
    EXPECT_FALSE(result.is_valid) << "Empty file should not be valid";
    EXPECT_EQ(result.file_size, 0) << "File size should be 0";
    EXPECT_THAT(result.error_message, ::testing::HasSubstr("empty")) << "Error should mention empty file";
}

TEST_F(AudioValidationTest, ValidateNonExistentFile)
{
    auto result = AudioValidationHelper::validateAudioFile("/non/existent/file.mp3");
    
    EXPECT_FALSE(result.is_valid) << "Non-existent file should not be valid";
    EXPECT_THAT(result.error_message, ::testing::HasSubstr("does not exist")) << "Error should mention file doesn't exist";
}

// Test directory validation
TEST_F(AudioValidationTest, ValidateDirectory)
{
    auto results = AudioValidationHelper::validateDirectory(test_output_dir.string());
    
    EXPECT_FALSE(results.empty()) << "Should find audio files in directory";
    
    // Count valid and invalid files
    size_t valid_count = 0;
    size_t invalid_count = 0;
    
    for (const auto& [file_path, result] : results) {
        if (result.is_valid) {
            valid_count++;
        } else {
            invalid_count++;
        }
    }
    
    EXPECT_GT(valid_count, 0) << "Should have some valid files";
    EXPECT_GT(invalid_count, 0) << "Should have some invalid files";
    
    std::cout << "Found " << valid_count << " valid and " << invalid_count << " invalid files" << std::endl;
}

TEST_F(AudioValidationTest, ValidateNonExistentDirectory)
{
    auto results = AudioValidationHelper::validateDirectory("/non/existent/directory");
    
    EXPECT_TRUE(results.empty()) << "Non-existent directory should return empty results";
}

// Test comparison functionality
TEST_F(AudioValidationTest, CompareWithOriginal)
{
    // Create a fake WAV file as "original"
    fs::path wav_file = test_output_dir / "original.wav";
    std::ofstream wav(wav_file, std::ios::binary);
    std::vector<uint8_t> wav_data(2048, 0x55); // Larger "original" file
    wav.write(reinterpret_cast<const char*>(wav_data.data()), wav_data.size());
    wav.close();
    
    fs::path mp3_file = test_output_dir / "test.mp3";
    auto result = AudioValidationHelper::compareWithOriginal(wav_file.string(), mp3_file.string());
    
    EXPECT_TRUE(result.is_valid) << "Comparison should succeed for valid encoded file";
    EXPECT_FALSE(result.metadata["compression_ratio"].empty()) << "Compression ratio should be calculated";
    EXPECT_FALSE(result.metadata["size_reduction"].empty()) << "Size reduction should be calculated";
    
    std::cout << "Compression ratio: " << result.metadata["compression_ratio"] << std::endl;
    std::cout << "Size reduction: " << result.metadata["size_reduction"] << std::endl;
}

// Test report generation
TEST_F(AudioValidationTest, GenerateValidationReport)
{
    auto results = AudioValidationHelper::validateDirectory(test_output_dir.string());
    std::string report = AudioValidationHelper::generateValidationReport(results);
    
    EXPECT_FALSE(report.empty()) << "Report should not be empty";
    EXPECT_THAT(report, ::testing::HasSubstr("Audio File Validation Report")) << "Report should have title";
    EXPECT_THAT(report, ::testing::HasSubstr("Summary:")) << "Report should have summary";
    EXPECT_THAT(report, ::testing::HasSubstr("VALID")) << "Report should show valid files";
    EXPECT_THAT(report, ::testing::HasSubstr("INVALID")) << "Report should show invalid files";
    
    std::cout << "\n" << report << std::endl;
}

// Test format detection edge cases
TEST_F(AudioValidationTest, FormatDetectionByExtension)
{
    // Test files with correct extensions but wrong content
    fs::path fake_mp3 = test_output_dir / "fake.mp3";
    std::ofstream file(fake_mp3, std::ios::binary);
    file.write("NotAnMP3File", 11);
    file.close();
    
    auto result = AudioValidationHelper::validateAudioFile(fake_mp3.string());
    
    // Should detect as MP3 by extension but fail validation
    EXPECT_FALSE(result.is_valid) << "File with wrong content should be invalid";
    EXPECT_EQ(result.format, AudioValidationHelper::AudioFormat::MP3) << "Format should be detected by extension";
}

// Performance test for large directory
TEST_F(AudioValidationTest, PerformanceWithManyFiles)
{
    // Create many test files
    for (int i = 0; i < 50; ++i) {
        fs::path file_path = test_output_dir / ("test_" + std::to_string(i) + ".mp3");
        std::ofstream file(file_path, std::ios::binary);
        
        // Alternate between valid and invalid files
        if (i % 2 == 0) {
            // Valid MP3-like file
            file.write("ID3", 3);
            file.write("\x03\x00\x00", 3);
            file.write("\x00\x00\x00\x00", 4);
            uint8_t mp3_frame[] = {0xFF, 0xFB, 0x90, 0x00};
            file.write(reinterpret_cast<const char*>(mp3_frame), sizeof(mp3_frame));
        } else {
            // Invalid file
            file.write("InvalidData", 11);
        }
        
        std::vector<uint8_t> dummy_data(100, static_cast<uint8_t>(i));
        file.write(reinterpret_cast<const char*>(dummy_data.data()), dummy_data.size());
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto results = AudioValidationHelper::validateDirectory(test_output_dir.string());
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_GE(results.size(), 50) << "Should validate all created files";
    
    std::cout << "Validated " << results.size() << " files in " << duration.count() << "ms" << std::endl;
    
    // Performance should be reasonable (less than 1 second for 50+ files)
    EXPECT_LT(duration.count(), 1000) << "Validation should complete within 1 second";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
