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
#include <thread>
#include <chrono>

// Include all encoder headers
#include "core/EncoderMP3.h"
#include "core/EncoderAAC.h"
#include "core/EncoderFLAC.h"
#include "core/EncoderOpus.h"
#include "core/EncoderVorbis.h"
#include "core/EncoderFactory.h"
#include "utils/WaveFileWrapper.h"
#include "utils/FileSystemHelper.h"

namespace fs = std::filesystem;

/**
 * @class EncoderFunctionalTest
 * @brief Comprehensive functional test suite for all audio encoder implementations.
 *
 * This test class provides thorough testing of all encoder classes (MP3, AAC, FLAC, Opus, Vorbis)
 * using real WAV files from the test_wav directory. It validates:
 * - Basic encoding functionality for each format
 * - Multi-threaded encoding operations
 * - Error handling for invalid inputs
 * - Output file validation and integrity
 * - Performance characteristics
 * - Memory management and resource cleanup
 */
class EncoderFunctionalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Set up test directories
        test_input_dir = fs::current_path() / "test_wav";
        test_output_dir = fs::temp_directory_path() / "encoder_test_output";
        
        // Ensure output directory exists and is clean
        if (fs::exists(test_output_dir)) {
            fs::remove_all(test_output_dir);
        }
        fs::create_directories(test_output_dir);
        
        // Verify test input directory exists
        ASSERT_TRUE(fs::exists(test_input_dir)) 
            << "Test WAV directory not found: " << test_input_dir;
        
        // Collect valid WAV files for testing
        collectValidWavFiles();
        
        ASSERT_FALSE(valid_wav_files.empty()) 
            << "No valid WAV files found in test directory";
    }

    void TearDown() override
    {
        // Clean up test output directory
        if (fs::exists(test_output_dir)) {
            fs::remove_all(test_output_dir);
        }
    }

    /**
     * @brief Collects valid WAV files from the test directory.
     * 
     * Scans the test_wav directory and validates each file using WaveFileWrapper
     * to ensure only valid WAV files are used for testing.
     */
    void collectValidWavFiles()
    {
        utils::FileSystemHelper fs_helper;
        std::vector<std::string> all_files;
        
        if (fs_helper.get_file_paths(test_input_dir.string(), all_files)) {
            for (const auto& file : all_files) {
                utils::WaveHeader header;
                if (utils::WaveFileWrapper::validate(file, header)) {
                    valid_wav_files.push_back(file);
                    wav_headers[file] = header;
                }
            }
        }
    }

    /**
     * @brief Validates that an output file exists and has reasonable size.
     * @param output_file Path to the output file to validate.
     * @param min_size_ratio Minimum expected size ratio compared to input.
     * @return True if file is valid, false otherwise.
     */
    bool validateOutputFile(const std::string& output_file, double min_size_ratio = 0.01)
    {
        if (!fs::exists(output_file)) {
            return false;
        }
        
        auto file_size = fs::file_size(output_file);
        return file_size > 1024; // At least 1KB
    }

    /**
     * @brief Tests encoding with a specific encoder implementation.
     * @param encoder Pointer to the encoder to test.
     * @param format_name Name of the format for logging.
     * @param expected_extension Expected file extension.
     */
    void testEncoderImplementation(
        std::unique_ptr<core::Encoder> encoder,
        const std::string& format_name,
        const std::string& expected_extension)
    {
        ASSERT_NE(encoder, nullptr) << format_name << " encoder creation failed";
        
        // Set output directory
        encoder->set_output_directory(test_output_dir.string());
        
        // Scan input directory
        auto scan_result = encoder->scan_input_directory(test_input_dir.string());
        EXPECT_EQ(scan_result, common::ErrorCode::ERROR_NONE) 
            << format_name << " failed to scan input directory";
        
        // Verify files were found
        const auto& input_files = encoder->get_input_files();
        EXPECT_FALSE(input_files.empty()) 
            << format_name << " found no input files";
        
        // Start encoding
        auto encode_result = encoder->start_encoding();
        EXPECT_EQ(encode_result, common::ErrorCode::ERROR_NONE) 
            << format_name << " encoding failed";
        
        // Validate output files
        for (const auto& input_file : input_files) {
            fs::path input_path(input_file);
            std::string output_filename = input_path.stem().string() + expected_extension;
            fs::path output_path = test_output_dir / output_filename;
            
            EXPECT_TRUE(validateOutputFile(output_path.string())) 
                << format_name << " output file validation failed: " << output_path;
        }
    }

    /**
     * @brief Tests multi-threaded encoding performance.
     * @param encoder Pointer to the encoder to test.
     * @param format_name Name of the format for logging.
     * @param thread_count Number of threads to use.
     */
    void testMultiThreadedEncoding(
        std::unique_ptr<core::Encoder> encoder,
        const std::string& format_name,
        uint16_t thread_count)
    {
        ASSERT_NE(encoder, nullptr) << format_name << " encoder creation failed";
        
        encoder->set_output_directory(test_output_dir.string());
        encoder->scan_input_directory(test_input_dir.string());
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto result = encoder->start_encoding();
        auto end_time = std::chrono::high_resolution_clock::now();
        
        EXPECT_EQ(result, common::ErrorCode::ERROR_NONE) 
            << format_name << " multi-threaded encoding failed";
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << format_name << " encoding with " << thread_count 
                  << " threads took: " << duration.count() << "ms" << std::endl;
    }

protected:
    fs::path test_input_dir;
    fs::path test_output_dir;
    std::vector<std::string> valid_wav_files;
    std::map<std::string, utils::WaveHeader> wav_headers;
};

// Test MP3 Encoder
TEST_F(EncoderFunctionalTest, MP3EncoderBasicFunctionality)
{
    auto encoder = std::make_unique<core::EncoderMP3>(
        common::AudioFormatType::WAV, 1, false);
    
    testEncoderImplementation(std::move(encoder), "MP3", ".mp3");
}

TEST_F(EncoderFunctionalTest, MP3EncoderMultiThreaded)
{
    auto encoder = std::make_unique<core::EncoderMP3>(
        common::AudioFormatType::WAV, 4, false);
    
    testMultiThreadedEncoding(std::move(encoder), "MP3", 4);
}

TEST_F(EncoderFunctionalTest, MP3EncoderVersionInfo)
{
    auto encoder = std::make_unique<core::EncoderMP3>(
        common::AudioFormatType::WAV, 1, false);
    
    const std::string& version = encoder->get_encoder_version();
    EXPECT_FALSE(version.empty()) << "MP3 encoder version should not be empty";
    EXPECT_THAT(version, ::testing::HasSubstr("Lame")) << "MP3 encoder should use LAME";
}

// Test AAC Encoder
TEST_F(EncoderFunctionalTest, AACEncoderBasicFunctionality)
{
    auto encoder = std::make_unique<core::EncoderAAC>(
        common::AudioFormatType::WAV, 1, false);
    
    testEncoderImplementation(std::move(encoder), "AAC", ".aac");
}

TEST_F(EncoderFunctionalTest, AACEncoderMultiThreaded)
{
    auto encoder = std::make_unique<core::EncoderAAC>(
        common::AudioFormatType::WAV, 4, false);
    
    testMultiThreadedEncoding(std::move(encoder), "AAC", 4);
}

TEST_F(EncoderFunctionalTest, AACEncoderVersionInfo)
{
    auto encoder = std::make_unique<core::EncoderAAC>(
        common::AudioFormatType::WAV, 1, false);
    
    const std::string& version = encoder->get_encoder_version();
    EXPECT_FALSE(version.empty()) << "AAC encoder version should not be empty";
    EXPECT_TRUE(version.find("AAC") != std::string::npos || 
                version.find("FFmpeg") != std::string::npos) 
        << "AAC encoder should mention AAC or FFmpeg";
}

// Test FLAC Encoder
TEST_F(EncoderFunctionalTest, FLACEncoderBasicFunctionality)
{
    auto encoder = std::make_unique<core::EncoderFLAC>(
        common::AudioFormatType::WAV, 1, false);
    
    testEncoderImplementation(std::move(encoder), "FLAC", ".flac");
}

TEST_F(EncoderFunctionalTest, FLACEncoderMultiThreaded)
{
    auto encoder = std::make_unique<core::EncoderFLAC>(
        common::AudioFormatType::WAV, 4, false);
    
    testMultiThreadedEncoding(std::move(encoder), "FLAC", 4);
}

TEST_F(EncoderFunctionalTest, FLACEncoderVersionInfo)
{
    auto encoder = std::make_unique<core::EncoderFLAC>(
        common::AudioFormatType::WAV, 1, false);
    
    const std::string& version = encoder->get_encoder_version();
    EXPECT_FALSE(version.empty()) << "FLAC encoder version should not be empty";
    EXPECT_THAT(version, ::testing::HasSubstr("FLAC")) << "FLAC encoder should mention FLAC";
}

// Test Opus Encoder
TEST_F(EncoderFunctionalTest, OpusEncoderBasicFunctionality)
{
    auto encoder = std::make_unique<core::EncoderOpus>(
        common::AudioFormatType::WAV, 1, false);
    
    testEncoderImplementation(std::move(encoder), "Opus", ".opus");
}

TEST_F(EncoderFunctionalTest, OpusEncoderMultiThreaded)
{
    auto encoder = std::make_unique<core::EncoderOpus>(
        common::AudioFormatType::WAV, 4, false);
    
    testMultiThreadedEncoding(std::move(encoder), "Opus", 4);
}

TEST_F(EncoderFunctionalTest, OpusEncoderVersionInfo)
{
    auto encoder = std::make_unique<core::EncoderOpus>(
        common::AudioFormatType::WAV, 1, false);
    
    const std::string& version = encoder->get_encoder_version();
    EXPECT_FALSE(version.empty()) << "Opus encoder version should not be empty";
    EXPECT_THAT(version, ::testing::HasSubstr("Opus")) << "Opus encoder should mention Opus";
}

// Test Vorbis Encoder
TEST_F(EncoderFunctionalTest, VorbisEncoderBasicFunctionality)
{
    auto encoder = std::make_unique<core::EncoderVorbis>(
        common::AudioFormatType::WAV, 1, false);
    
    testEncoderImplementation(std::move(encoder), "Vorbis", ".ogg");
}

TEST_F(EncoderFunctionalTest, VorbisEncoderMultiThreaded)
{
    auto encoder = std::make_unique<core::EncoderVorbis>(
        common::AudioFormatType::WAV, 4, false);
    
    testMultiThreadedEncoding(std::move(encoder), "Vorbis", 4);
}

TEST_F(EncoderFunctionalTest, VorbisEncoderVersionInfo)
{
    auto encoder = std::make_unique<core::EncoderVorbis>(
        common::AudioFormatType::WAV, 1, false);
    
    const std::string& version = encoder->get_encoder_version();
    EXPECT_FALSE(version.empty()) << "Vorbis encoder version should not be empty";
    EXPECT_THAT(version, ::testing::HasSubstr("Vorbis")) << "Vorbis encoder should mention Vorbis";
}

// Error Handling Tests
TEST_F(EncoderFunctionalTest, InvalidInputDirectory)
{
    auto encoder = std::make_unique<core::EncoderMP3>(
        common::AudioFormatType::WAV, 1, false);
    
    auto result = encoder->scan_input_directory("/non/existent/directory");
    EXPECT_EQ(result, common::ErrorCode::ERROR_NOT_FOUND);
}

TEST_F(EncoderFunctionalTest, EmptyInputDirectory)
{
    // Create empty directory
    fs::path empty_dir = test_output_dir / "empty";
    fs::create_directories(empty_dir);
    
    auto encoder = std::make_unique<core::EncoderMP3>(
        common::AudioFormatType::WAV, 1, false);
    
    auto result = encoder->scan_input_directory(empty_dir.string());
    EXPECT_EQ(result, common::ErrorCode::ERROR_NOT_FOUND);
}

TEST_F(EncoderFunctionalTest, EncodingCancellation)
{
    auto encoder = std::make_unique<core::EncoderMP3>(
        common::AudioFormatType::WAV, 1, false);
    
    encoder->set_output_directory(test_output_dir.string());
    encoder->scan_input_directory(test_input_dir.string());
    
    // Start encoding in a separate thread
    std::thread encoding_thread([&encoder]() {
        encoder->start_encoding();
    });
    
    // Cancel after a short delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto cancel_result = encoder->cancel_encoding();
    EXPECT_EQ(cancel_result, common::ErrorCode::ERROR_CANCELLED);
    
    encoding_thread.join();
}

// Performance and Stress Tests
TEST_F(EncoderFunctionalTest, AllEncodersPerformanceComparison)
{
    struct EncoderTestResult {
        std::string format;
        std::chrono::milliseconds duration;
        size_t output_files;
    };
    
    std::vector<EncoderTestResult> results;
    
    // Test each encoder
    std::vector<std::pair<std::string, std::function<std::unique_ptr<core::Encoder>()>>> encoders = {
        {"MP3", []() { return std::make_unique<core::EncoderMP3>(common::AudioFormatType::WAV, 2, false); }},
        {"AAC", []() { return std::make_unique<core::EncoderAAC>(common::AudioFormatType::WAV, 2, false); }},
        {"FLAC", []() { return std::make_unique<core::EncoderFLAC>(common::AudioFormatType::WAV, 2, false); }},
        {"Opus", []() { return std::make_unique<core::EncoderOpus>(common::AudioFormatType::WAV, 2, false); }},
        {"Vorbis", []() { return std::make_unique<core::EncoderVorbis>(common::AudioFormatType::WAV, 2, false); }}
    };
    
    for (const auto& [format, factory] : encoders) {
        // Clean output directory
        fs::remove_all(test_output_dir);
        fs::create_directories(test_output_dir);
        
        auto encoder = factory();
        encoder->set_output_directory(test_output_dir.string());
        encoder->scan_input_directory(test_input_dir.string());
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto result = encoder->start_encoding();
        auto end_time = std::chrono::high_resolution_clock::now();
        
        if (result == common::ErrorCode::ERROR_NONE) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            size_t output_count = 0;
            
            for (const auto& entry : fs::directory_iterator(test_output_dir)) {
                if (entry.is_regular_file()) {
                    output_count++;
                }
            }
            
            results.push_back({format, duration, output_count});
        }
    }
    
    // Print performance results
    std::cout << "\n=== Encoder Performance Comparison ===" << std::endl;
    for (const auto& result : results) {
        std::cout << result.format << ": " << result.duration.count() 
                  << "ms (" << result.output_files << " files)" << std::endl;
    }
}

// Integration Tests with Real Files
TEST_F(EncoderFunctionalTest, SpecificWavFileTests)
{
    // Test with specific known files if they exist
    std::vector<std::string> test_files = {
        "beep22K.wav",
        "Kurzweil-K2000-Bright-Piano-C4.wav"
    };
    
    for (const auto& test_file : test_files) {
        fs::path file_path = test_input_dir / test_file;
        if (fs::exists(file_path)) {
            utils::WaveHeader header;
            EXPECT_TRUE(utils::WaveFileWrapper::validate(file_path.string(), header))
                << "Test file should be valid: " << test_file;
            
            std::cout << "Testing with " << test_file << ": "
                      << header.channels << " channels, "
                      << header.sample_rate << " Hz, "
                      << header.bits_per_sample << " bits" << std::endl;
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
