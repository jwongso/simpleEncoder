#ifndef ENCODER_FACTORY_H
#define ENCODER_FACTORY_H

#include "Encoder.h"
#include "EncoderFLAC.h"
#include "EncoderMP3.h"
#include "EncoderOpus.h"
#include "EncoderAAC.h"
#include "common/Common.h"
#include <memory>
#include <string>

namespace core {

class EncoderFactory {
public:
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
            default:
                return nullptr;
        }
    }

    static std::string format_type_to_string(common::AudioFormatType format)
    {
        switch (format) {
            case common::AudioFormatType::WAV:  return "wav";
            case common::AudioFormatType::FLAC: return "flac";
            case common::AudioFormatType::MP3:  return "mp3";
            case common::AudioFormatType::OPUS: return "opus";
            case common::AudioFormatType::AAC:  return "aac";
            default: return "unknown";
        }
    }

    static common::AudioFormatType string_to_format_type(const std::string& format)
    {
        if (format == "wav")  return common::AudioFormatType::WAV;
        if (format == "flac") return common::AudioFormatType::FLAC;
        if (format == "mp3")  return common::AudioFormatType::MP3;
        if (format == "opus") return common::AudioFormatType::OPUS;
        if (format == "aac")  return common::AudioFormatType::AAC;
        return common::AudioFormatType::UNKNOWN;
    }

    static std::vector<std::string> get_supported_formats()
    {
        return {"flac", "mp3", "opus", "aac"};
    }
};

} // namespace core

#endif // ENCODER_FACTORY_H
