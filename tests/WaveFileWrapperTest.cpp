#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "utils/WaveFileWrapper.h"
#include <memory>
#include <fstream>
#include <filesystem>

namespace utils {
namespace testing {

// Mock for FileSystemHelper to make WaveFileWrapper testable
class MockFileSystemHelper {
public:
    MOCK_METHOD(bool, read_binary_file, (const std::string&, std::vector<uint8_t>&), ());
};

class WaveFileWrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file for integration tests
        test_filename = "test_wave_file.wav";
    }

    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_filename)) {
            std::filesystem::remove(test_filename);
        }
    }

    std::string test_filename;

    // Helper to create a valid WAVE file header
    std::vector<uint8_t> createValidWaveHeader(uint16_t channels = 2,
                                              uint32_t sample_rate = 44100,
                                              uint16_t bits_per_sample = 16,
                                              uint32_t data_size = 1000) {
        std::vector<uint8_t> header;

        // RIFF header
        header.insert(header.end(), {'R', 'I', 'F', 'F'});

        // File length (chunk size) - will be calculated
        uint32_t file_length = 36 + data_size;
        header.push_back(file_length & 0xFF);
        header.push_back((file_length >> 8) & 0xFF);
        header.push_back((file_length >> 16) & 0xFF);
        header.push_back((file_length >> 24) & 0xFF);

        // WAVE header
        header.insert(header.end(), {'W', 'A', 'V', 'E'});

        // FMT chunk
        header.insert(header.end(), {'f', 'm', 't', ' '});

        // FMT chunk size (16 for PCM)
        uint32_t fmt_size = 16;
        header.push_back(fmt_size & 0xFF);
        header.push_back((fmt_size >> 8) & 0xFF);
        header.push_back((fmt_size >> 16) & 0xFF);
        header.push_back((fmt_size >> 24) & 0xFF);

        // Audio format (1 = PCM)
        uint16_t format = 1;
        header.push_back(format & 0xFF);
        header.push_back((format >> 8) & 0xFF);

        // Number of channels
        header.push_back(channels & 0xFF);
        header.push_back((channels >> 8) & 0xFF);

        // Sample rate
        header.push_back(sample_rate & 0xFF);
        header.push_back((sample_rate >> 8) & 0xFF);
        header.push_back((sample_rate >> 16) & 0xFF);
        header.push_back((sample_rate >> 24) & 0xFF);

        // Bytes per second
        uint32_t bytes_per_sec = sample_rate * channels * (bits_per_sample / 8);
        header.push_back(bytes_per_sec & 0xFF);
        header.push_back((bytes_per_sec >> 8) & 0xFF);
        header.push_back((bytes_per_sec >> 16) & 0xFF);
        header.push_back((bytes_per_sec >> 24) & 0xFF);

        // Block align
        uint16_t block_align = channels * (bits_per_sample / 8);
        header.push_back(block_align & 0xFF);
        header.push_back((block_align >> 8) & 0xFF);

        // Bits per sample
        header.push_back(bits_per_sample & 0xFF);
        header.push_back((bits_per_sample >> 8) & 0xFF);

        // DATA chunk
        header.insert(header.end(), {'d', 'a', 't', 'a'});

        // Data size
        header.push_back(data_size & 0xFF);
        header.push_back((data_size >> 8) & 0xFF);
        header.push_back((data_size >> 16) & 0xFF);
        header.push_back((data_size >> 24) & 0xFF);

        return header;
    }

    std::vector<uint8_t> createInvalidWaveHeader() {
        std::vector<uint8_t> header;
        // Invalid RIFF header
        header.insert(header.end(), {'X', 'I', 'F', 'F'});
        // Add some more bytes to make it larger than MIN_HEADER_SIZE
        header.resize(50, 0);
        return header;
    }

    void createTestWaveFile(const std::vector<uint8_t>& header_data,
                           const std::vector<uint8_t>& audio_data = {}) {
        std::ofstream file(test_filename, std::ios::binary);
        file.write(reinterpret_cast<const char*>(header_data.data()), header_data.size());
        if (!audio_data.empty()) {
            file.write(reinterpret_cast<const char*>(audio_data.data()), audio_data.size());
        }
        file.close();
    }
};

// Test WaveFileWrapper constructor and validation
TEST_F(WaveFileWrapperTest, ConstructorWithValidFile) {
    auto header = createValidWaveHeader();
    createTestWaveFile(header);

    WaveFileWrapper wrapper(test_filename);
    EXPECT_TRUE(wrapper.is_valid());
}

TEST_F(WaveFileWrapperTest, ConstructorWithInvalidFile) {
    auto header = createInvalidWaveHeader();
    createTestWaveFile(header);

    WaveFileWrapper wrapper(test_filename);
    EXPECT_FALSE(wrapper.is_valid());
}

TEST_F(WaveFileWrapperTest, ConstructorWithNonexistentFile) {
    WaveFileWrapper wrapper("nonexistent_file.wav");
    EXPECT_FALSE(wrapper.is_valid());
}

// Test static validate method
TEST_F(WaveFileWrapperTest, ValidateValidWaveFile) {
    auto header = createValidWaveHeader(2, 44100, 16, 1000);
    createTestWaveFile(header);

    WaveHeader parsed_header;
    bool result = WaveFileWrapper::validate(test_filename, parsed_header);

    EXPECT_TRUE(result);
    EXPECT_EQ(std::string(parsed_header.riff, 4), "RIFF");
    EXPECT_EQ(std::string(parsed_header.wave, 4), "WAVE");
    EXPECT_EQ(std::string(parsed_header.fmt, 4), "fmt ");
    EXPECT_EQ(std::string(parsed_header.data, 4), "data");
    EXPECT_EQ(parsed_header.channels, 2);
    EXPECT_EQ(parsed_header.sample_rate, 44100);
    EXPECT_EQ(parsed_header.bits_per_sample, 16);
    EXPECT_EQ(parsed_header.format, 1);
    EXPECT_EQ(parsed_header.data_size, 1000);
}

TEST_F(WaveFileWrapperTest, ValidateInvalidRiffHeader) {
    auto header = createInvalidWaveHeader();
    createTestWaveFile(header);

    WaveHeader parsed_header;
    bool result = WaveFileWrapper::validate(test_filename, parsed_header);

    EXPECT_FALSE(result);
}

TEST_F(WaveFileWrapperTest, ValidateFileTooSmall) {
    std::vector<uint8_t> small_header(10, 0x00); // Smaller than MIN_HEADER_SIZE
    createTestWaveFile(small_header);

    WaveHeader parsed_header;
    bool result = WaveFileWrapper::validate(test_filename, parsed_header);

    EXPECT_FALSE(result);
}

TEST_F(WaveFileWrapperTest, ValidateNonexistentFile) {
    WaveHeader parsed_header;
    bool result = WaveFileWrapper::validate("nonexistent.wav", parsed_header);

    EXPECT_FALSE(result);
}

// Test mono vs stereo handling
TEST_F(WaveFileWrapperTest, ValidateMonoFile) {
    auto header = createValidWaveHeader(1, 44100, 16, 500); // Mono
    createTestWaveFile(header);

    WaveHeader parsed_header;
    bool result = WaveFileWrapper::validate(test_filename, parsed_header);

    EXPECT_TRUE(result);
    EXPECT_EQ(parsed_header.channels, 1);
}

// Test different sample rates and bit depths
TEST_F(WaveFileWrapperTest, ValidateDifferentSampleRates) {
    auto header = createValidWaveHeader(2, 48000, 24, 2000); // 48kHz, 24-bit
    createTestWaveFile(header);

    WaveHeader parsed_header;
    bool result = WaveFileWrapper::validate(test_filename, parsed_header);

    EXPECT_TRUE(result);
    EXPECT_EQ(parsed_header.sample_rate, 48000);
    EXPECT_EQ(parsed_header.bits_per_sample, 24);
}

// Test get_wave_data method
TEST_F(WaveFileWrapperTest, GetWaveDataValidFile) {
    auto header = createValidWaveHeader(2, 44100, 16, 8); // 2 samples of stereo 16-bit
    std::vector<uint8_t> audio_data = {
        0x00, 0x01, 0x00, 0x02,  // Sample 1: left=0x0100, right=0x0200
        0x00, 0x03, 0x00, 0x04   // Sample 2: left=0x0300, right=0x0400
    };
    createTestWaveFile(header, audio_data);

    WaveFileWrapper wrapper(test_filename);
    ASSERT_TRUE(wrapper.is_valid());

    WaveHeader header_data;
    int16_t* left = nullptr;
    int16_t* right = nullptr;

    bool result = wrapper.get_wave_data(header_data, left, right);

    EXPECT_TRUE(result);
    EXPECT_NE(left, nullptr);
    EXPECT_NE(right, nullptr);

    // Verify some audio data (note: values depend on endianness)
    EXPECT_EQ(left[0], 0x0100);
    EXPECT_EQ(right[0], 0x0200);

    // Clean up allocated memory
    delete[] left;
    delete[] right;
}

TEST_F(WaveFileWrapperTest, GetWaveDataInvalidFile) {
    WaveFileWrapper wrapper("nonexistent.wav");
    EXPECT_FALSE(wrapper.is_valid());

    WaveHeader header_data;
    int16_t* left = nullptr;
    int16_t* right = nullptr;

    bool result = wrapper.get_wave_data(header_data, left, right);

    EXPECT_FALSE(result);
    EXPECT_EQ(left, nullptr);
    EXPECT_EQ(right, nullptr);
}

// Test memory management issues
TEST_F(WaveFileWrapperTest, GetWaveDataMemoryAllocation) {
    auto header = createValidWaveHeader(1, 44100, 16, 4); // Mono, 2 samples
    std::vector<uint8_t> audio_data = {0x00, 0x01, 0x00, 0x02};
    createTestWaveFile(header, audio_data);

    WaveFileWrapper wrapper(test_filename);
    ASSERT_TRUE(wrapper.is_valid());

    WaveHeader header_data;
    int16_t* left = nullptr;
    int16_t* right = nullptr;

    bool result = wrapper.get_wave_data(header_data, left, right);

    EXPECT_TRUE(result);
    EXPECT_NE(left, nullptr);
    EXPECT_EQ(right, nullptr); // Should be null for mono

    // Clean up
    delete[] left;
}

} // namespace testing
} // namespace utils
