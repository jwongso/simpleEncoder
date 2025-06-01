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

#ifndef AUDIO_VALIDATION_HELPER_H
#define AUDIO_VALIDATION_HELPER_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <algorithm>

namespace fs = std::filesystem;

/**
 * @class AudioValidationHelper
 * @brief Utility class for validating encoded audio files and their properties.
 *
 * This class provides comprehensive validation capabilities for encoded audio files,
 * including format detection, basic integrity checks, and metadata validation.
 * It supports validation of MP3, AAC, FLAC, Opus, and Vorbis files.
 */
class AudioValidationHelper
{
public:
    /**
     * @enum AudioFormat
     * @brief Enumeration of supported audio formats for validation.
     */
    enum class AudioFormat {
        UNKNOWN,
        MP3,
        AAC,
        FLAC,
        OPUS,
        VORBIS
    };

    /**
     * @struct ValidationResult
     * @brief Structure containing validation results for an audio file.
     */
    struct ValidationResult {
        bool is_valid = false;
        AudioFormat format = AudioFormat::UNKNOWN;
        std::string error_message;
        size_t file_size = 0;
        std::map<std::string, std::string> metadata;
        
        // Basic audio properties (if detectable)
        uint32_t sample_rate = 0;
        uint16_t channels = 0;
        uint32_t bitrate = 0;
        double duration_seconds = 0.0;
    };

    /**
     * @brief Validates an audio file and returns detailed results.
     * @param file_path Path to the audio file to validate.
     * @return ValidationResult containing validation status and metadata.
     */
    static ValidationResult validateAudioFile(const std::string& file_path)
    {
        ValidationResult result;
        
        if (!fs::exists(file_path)) {
            result.error_message = "File does not exist";
            return result;
        }
        
        result.file_size = fs::file_size(file_path);
        if (result.file_size == 0) {
            result.error_message = "File is empty";
            return result;
        }
        
        // Detect format based on file extension and magic bytes
        result.format = detectAudioFormat(file_path);
        
        switch (result.format) {
            case AudioFormat::MP3:
                result = validateMP3File(file_path, result);
                break;
            case AudioFormat::AAC:
                result = validateAACFile(file_path, result);
                break;
            case AudioFormat::FLAC:
                result = validateFLACFile(file_path, result);
                break;
            case AudioFormat::OPUS:
                result = validateOpusFile(file_path, result);
                break;
            case AudioFormat::VORBIS:
                result = validateVorbisFile(file_path, result);
                break;
            default:
                result.error_message = "Unknown or unsupported audio format";
                return result;
        }
        
        return result;
    }

    /**
     * @brief Validates multiple audio files in a directory.
     * @param directory_path Path to directory containing audio files.
     * @param recursive Whether to search recursively in subdirectories.
     * @return Map of file paths to validation results.
     */
    static std::map<std::string, ValidationResult> validateDirectory(
        const std::string& directory_path, 
        bool recursive = false)
    {
        std::map<std::string, ValidationResult> results;
        
        if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
            return results;
        }
        
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(directory_path)) {
                if (entry.is_regular_file()) {
                    std::string file_path = entry.path().string();
                    if (isAudioFile(file_path)) {
                        results[file_path] = validateAudioFile(file_path);
                    }
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(directory_path)) {
                if (entry.is_regular_file()) {
                    std::string file_path = entry.path().string();
                    if (isAudioFile(file_path)) {
                        results[file_path] = validateAudioFile(file_path);
                    }
                }
            }
        }
        
        return results;
    }

    /**
     * @brief Compares encoded file properties with original WAV file.
     * @param original_wav_path Path to original WAV file.
     * @param encoded_file_path Path to encoded audio file.
     * @return ValidationResult with comparison information.
     */
    static ValidationResult compareWithOriginal(
        const std::string& original_wav_path,
        const std::string& encoded_file_path)
    {
        ValidationResult result = validateAudioFile(encoded_file_path);
        
        if (!result.is_valid) {
            return result;
        }
        
        // Add comparison metadata
        result.metadata["original_file"] = original_wav_path;
        result.metadata["encoded_file"] = encoded_file_path;
        
        if (fs::exists(original_wav_path)) {
            size_t original_size = fs::file_size(original_wav_path);
            double compression_ratio = static_cast<double>(result.file_size) / original_size;
            result.metadata["compression_ratio"] = std::to_string(compression_ratio);
            result.metadata["size_reduction"] = std::to_string((1.0 - compression_ratio) * 100.0) + "%";
        }
        
        return result;
    }

    /**
     * @brief Generates a validation report for multiple files.
     * @param results Map of validation results.
     * @return String containing formatted validation report.
     */
    static std::string generateValidationReport(
        const std::map<std::string, ValidationResult>& results)
    {
        std::ostringstream report;
        
        report << "=== Audio File Validation Report ===\n\n";
        
        size_t valid_files = 0;
        size_t total_files = results.size();
        
        for (const auto& [file_path, result] : results) {
            fs::path path(file_path);
            report << "File: " << path.filename().string() << "\n";
            report << "  Status: " << (result.is_valid ? "VALID" : "INVALID") << "\n";
            
            if (result.is_valid) {
                valid_files++;
                report << "  Format: " << formatToString(result.format) << "\n";
                report << "  Size: " << result.file_size << " bytes\n";
                
                if (result.sample_rate > 0) {
                    report << "  Sample Rate: " << result.sample_rate << " Hz\n";
                }
                if (result.channels > 0) {
                    report << "  Channels: " << result.channels << "\n";
                }
                if (result.bitrate > 0) {
                    report << "  Bitrate: " << result.bitrate << " bps\n";
                }
                if (result.duration_seconds > 0) {
                    report << "  Duration: " << result.duration_seconds << " seconds\n";
                }
                
                // Include metadata
                for (const auto& [key, value] : result.metadata) {
                    if (!value.empty()) {
                        report << "  " << key << ": " << value << "\n";
                    }
                }
            } else {
                report << "  Error: " << result.error_message << "\n";
            }
            report << "\n";
        }
        
        report << "Summary: " << valid_files << "/" << total_files << " files valid\n";
        
        return report.str();
    }

private:
    /**
     * @brief Detects audio format based on file extension and magic bytes.
     * @param file_path Path to the audio file.
     * @return Detected AudioFormat.
     */
    static AudioFormat detectAudioFormat(const std::string& file_path)
    {
        fs::path path(file_path);
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        // Check by extension first
        if (extension == ".mp3") return AudioFormat::MP3;
        if (extension == ".aac") return AudioFormat::AAC;
        if (extension == ".flac") return AudioFormat::FLAC;
        if (extension == ".opus") return AudioFormat::OPUS;
        if (extension == ".ogg") return AudioFormat::VORBIS;
        
        // Check magic bytes for more reliable detection
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return AudioFormat::UNKNOWN;
        }
        
        std::vector<uint8_t> header(16);
        file.read(reinterpret_cast<char*>(header.data()), header.size());
        
        // MP3 magic bytes (ID3 tag or frame sync)
        if (header.size() >= 3 && 
            ((header[0] == 'I' && header[1] == 'D' && header[2] == '3') ||
             (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0))) {
            return AudioFormat::MP3;
        }
        
        // FLAC magic bytes
        if (header.size() >= 4 && 
            header[0] == 'f' && header[1] == 'L' && header[2] == 'a' && header[3] == 'C') {
            return AudioFormat::FLAC;
        }
        
        // OGG magic bytes (could be Vorbis or Opus)
        if (header.size() >= 4 && 
            header[0] == 'O' && header[1] == 'g' && header[2] == 'g' && header[3] == 'S') {
            // Need to check further to distinguish Vorbis from Opus
            return detectOggFormat(file_path);
        }
        
        // AAC ADTS magic bytes
        if (header.size() >= 2 && 
            header[0] == 0xFF && (header[1] & 0xF0) == 0xF0) {
            return AudioFormat::AAC;
        }
        
        return AudioFormat::UNKNOWN;
    }

    /**
     * @brief Distinguishes between Vorbis and Opus in OGG container.
     * @param file_path Path to the OGG file.
     * @return AudioFormat (VORBIS or OPUS).
     */
    static AudioFormat detectOggFormat(const std::string& file_path)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return AudioFormat::UNKNOWN;
        }
        
        // Read more data to find codec identification
        std::vector<uint8_t> buffer(1024);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        // Look for Opus identification
        std::string buffer_str(buffer.begin(), buffer.end());
        if (buffer_str.find("OpusHead") != std::string::npos) {
            return AudioFormat::OPUS;
        }
        
        // Look for Vorbis identification
        if (buffer_str.find("vorbis") != std::string::npos) {
            return AudioFormat::VORBIS;
        }
        
        return AudioFormat::VORBIS; // Default to Vorbis for OGG
    }

    /**
     * @brief Validates MP3 file format and extracts basic information.
     */
    static ValidationResult validateMP3File(const std::string& file_path, ValidationResult result)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "Cannot open MP3 file";
            return result;
        }
        
        // Basic MP3 validation - check for valid frame sync
        std::vector<uint8_t> buffer(4);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        // Skip ID3 tag if present
        if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3') {
            // Skip ID3v2 tag
            file.seekg(6);
            file.read(reinterpret_cast<char*>(buffer.data()), 4);
            uint32_t tag_size = ((buffer[0] & 0x7F) << 21) | 
                               ((buffer[1] & 0x7F) << 14) | 
                               ((buffer[2] & 0x7F) << 7) | 
                               (buffer[3] & 0x7F);
            file.seekg(10 + tag_size);
            file.read(reinterpret_cast<char*>(buffer.data()), 4);
        }
        
        // Check for MP3 frame sync
        if (buffer[0] == 0xFF && (buffer[1] & 0xE0) == 0xE0) {
            result.is_valid = true;
            result.metadata["format_details"] = "Valid MP3 frame sync found";
        } else {
            result.error_message = "Invalid MP3 frame sync";
        }
        
        return result;
    }

    /**
     * @brief Validates AAC file format.
     */
    static ValidationResult validateAACFile(const std::string& file_path, ValidationResult result)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "Cannot open AAC file";
            return result;
        }
        
        std::vector<uint8_t> buffer(2);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        // Check for ADTS sync word
        if (buffer[0] == 0xFF && (buffer[1] & 0xF0) == 0xF0) {
            result.is_valid = true;
            result.metadata["format_details"] = "Valid AAC ADTS sync found";
        } else {
            result.error_message = "Invalid AAC ADTS sync";
        }
        
        return result;
    }

    /**
     * @brief Validates FLAC file format.
     */
    static ValidationResult validateFLACFile(const std::string& file_path, ValidationResult result)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "Cannot open FLAC file";
            return result;
        }
        
        std::vector<uint8_t> buffer(4);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        // Check for FLAC signature
        if (buffer[0] == 'f' && buffer[1] == 'L' && buffer[2] == 'a' && buffer[3] == 'C') {
            result.is_valid = true;
            result.metadata["format_details"] = "Valid FLAC signature found";
        } else {
            result.error_message = "Invalid FLAC signature";
        }
        
        return result;
    }

    /**
     * @brief Validates Opus file format.
     */
    // static ValidationResult validateOpusFile(const std::string& file_path, ValidationResult result)
    // {
    //     std::ifstream file(file_path, std::ios::binary);
    //     if (!file.is_open()) {
    //         result.error_message = "Cannot open Opus file";
    //         return result;
    //     }
        
    //     std::vector<uint8_t> buffer(1024);
    //     file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
    //     std::string buffer_str(buffer.begin(), buffer.end());
    //     if (buffer_str.find("OpusHead") != std::string::npos) {
    //         result.is_valid = true;
    //         result.metadata["format_details"] = "Valid Opus identification found";
    //     } else {
    //         result.error_message = "Opus identification not found";
    //     }
        
    //     return result;
    // }
    static ValidationResult validateOpusFile(const std::string& file_path, ValidationResult result)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "Cannot open Opus file";
            return result;
        }

        const std::string target = "OpusHead";
        const size_t chunk_size = 4096;
        std::vector<uint8_t> buffer(chunk_size);
        bool found = false;

        while (file) {
            file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            std::streamsize bytes_read = file.gcount();
            if (bytes_read == 0) {
                break; // EOF
            }

            auto it = std::search(buffer.begin(), buffer.begin() + bytes_read,
                                target.begin(), target.end());
            if (it != buffer.begin() + bytes_read) {
                found = true;
                break;
            }
        }

        if (found) {
            result.is_valid = true;
            result.metadata["format_details"] = "Valid Opus identification found";
        } else {
            result.error_message = "Opus identification not found";
        }

        return result;
    }

    /**
     * @brief Validates Vorbis file format.
     */
    static ValidationResult validateVorbisFile(const std::string& file_path, ValidationResult result)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "Cannot open Vorbis file";
            return result;
        }
        
        std::vector<uint8_t> buffer(1024);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        std::string buffer_str(buffer.begin(), buffer.end());
        if (buffer_str.find("vorbis") != std::string::npos) {
            result.is_valid = true;
            result.metadata["format_details"] = "Valid Vorbis identification found";
        } else {
            result.error_message = "Vorbis identification not found";
        }
        
        return result;
    }

    /**
     * @brief Checks if a file is likely an audio file based on extension.
     */
    static bool isAudioFile(const std::string& file_path)
    {
        fs::path path(file_path);
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        return extension == ".mp3" || extension == ".aac" || extension == ".flac" || 
               extension == ".opus" || extension == ".ogg";
    }

    /**
     * @brief Converts AudioFormat enum to string.
     */
    static std::string formatToString(AudioFormat format)
    {
        switch (format) {
            case AudioFormat::MP3: return "MP3";
            case AudioFormat::AAC: return "AAC";
            case AudioFormat::FLAC: return "FLAC";
            case AudioFormat::OPUS: return "Opus";
            case AudioFormat::VORBIS: return "Vorbis";
            default: return "Unknown";
        }
    }
};

#endif // AUDIO_VALIDATION_HELPER_H
