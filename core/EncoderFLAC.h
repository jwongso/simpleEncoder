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

#ifndef ENCODER_FLAC_H
#define ENCODER_FLAC_H

#include "Encoder.h"
#include <mutex>
#include <functional>
#include <vector>
#include <map>

namespace core
{

class EncoderFLAC : public Encoder
{
public:
    EncoderFLAC(common::AudioFormatType input_type, uint16_t thread_number);
    virtual ~EncoderFLAC();

    const std::string& get_encoder_version() const;

    virtual common::ErrorCode start_encoding() override;
    virtual common::ErrorCode cancel_encoding() override;

private:
    struct EncoderThreadArg
    {
        uint32_t thread_id;
        std::map<std::string, bool>* input_files;
        bool* cancelled;
        std::function<void(const std::string&, const std::string&)> callback;
    };

    static void* processing_files(void* arg);
    void on_encoding_status(const std::string& key, const std::string& value);
    static std::string get_flac_version();

private:
    std::string m_encoder_version;
    uint16_t m_thread_number;
    bool m_cancelled;
    std::map<std::string, bool> m_to_be_encoded_files;
    std::mutex m_mutex;
    std::vector<std::string> m_status;
};

} // core

#endif // ENCODER_FLAC_H
