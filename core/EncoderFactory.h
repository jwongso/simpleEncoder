#ifndef ENCODER_FACTORY_H
#define ENCODER_FACTORY_H

#include "Encoder.h"
#include "EncoderFLAC.h"
#include "EncoderMP3.h"
#include "EncoderOpus.h"
#include "EncoderAAC.h"
#include "EncoderVorbis.h"
#include "common/Common.h"
#include <memory>
#include <string>

/**
 * @class EncoderFactory
 * @brief Factory class for creating encoder instances based on output format.
 *
 * Provides static methods to instantiate format-specific encoders (e.g., FLAC, MP3)
 * and utility functions for format conversions. Ensures encoders are created uniformly.
 *
 * @note All methods are static; no need to instantiate this class.
 * @see Encoder
 */
namespace core {

class EncoderFactory {
public:
    /**
     * @brief Creates a unique pointer to an Encoder instance for the specified format.
     * @param output_format The target output audio format (e.g., FLAC).
     * @param input_format The input audio format (default: WAV).
     * @param thread_number Number of threads for encoding (default: 1).
     * @param verbose If true, enables verbose logging (default: false).
     * @return A std::unique_ptr to the Encoder, or nullptr if the format is unsupported.
     * @note Returns derived Encoder types (e.g., EncoderFLAC) based on output_format.
     */
    static std::unique_ptr<Encoder> create_encoder(
        common::AudioFormatType output_format,
        common::AudioFormatType input_format = common::AudioFormatType::WAV,
        uint16_t thread_number = 1,
        bool verbose = false)
    {
        switch (output_format) {
            case common::AudioFormatType::FLAC:
                return std::make_unique<EncoderFLAC>(input_format, thread_number, verbose);
            case common::AudioFormatType::MP3:
                return std::make_unique<EncoderMP3>(input_format, thread_number, verbose);
            case common::AudioFormatType::OPUS:
                return std::make_unique<EncoderOpus>(input_format, thread_number, verbose);
            case common::AudioFormatType::AAC:
                return std::make_unique<EncoderAAC>(input_format, thread_number, verbose);
            case common::AudioFormatType::VORBIS:
                return std::make_unique<EncoderVorbis>(input_format, thread_number, verbose);
            default:
                return nullptr;
        }
    }

    /**
     * @brief Converts an AudioFormatType enum to its string representation.
     * @param format The audio format enum value.
     * @return A string representing the format (e.g., "flac"), or "unknown" if invalid.
     */
    static std::string format_type_to_string(common::AudioFormatType format)
    {
        switch (format) {
            case common::AudioFormatType::WAV:  return "wav";
            case common::AudioFormatType::FLAC: return "flac";
            case common::AudioFormatType::MP3:  return "mp3";
            case common::AudioFormatType::OPUS: return "opus";
            case common::AudioFormatType::AAC:  return "aac";
            case common::AudioFormatType::VORBIS: return "vorbis";
            default: return "unknown";
        }
    }

    /**
     * @brief Converts a string to the corresponding AudioFormatType enum.
     * @param format The string representation of the format (e.g., "mp3").
     * @return The matching AudioFormatType, or UNKNOWN if not recognized.
     * @note Case-sensitive; expects lowercase strings as per get_supported_formats().
     */
    static common::AudioFormatType string_to_format_type(const std::string& format)
    {
        if (format == "wav")  return common::AudioFormatType::WAV;
        if (format == "flac") return common::AudioFormatType::FLAC;
        if (format == "mp3")  return common::AudioFormatType::MP3;
        if (format == "opus") return common::AudioFormatType::OPUS;
        if (format == "aac")  return common::AudioFormatType::AAC;
        if (format == "vorbis") return common::AudioFormatType::VORBIS;
        return common::AudioFormatType::UNKNOWN;
    }

    /**
     * @brief Retrieves a list of supported output formats as strings.
     * @return A vector of supported format strings (e.g., {"flac", "mp3"}).
     */
    static std::vector<std::string> get_supported_formats()
    {
        return {"flac", "mp3", "opus", "aac", "vorbis"};
    }
};

} // namespace core

#endif // ENCODER_FACTORY_H
