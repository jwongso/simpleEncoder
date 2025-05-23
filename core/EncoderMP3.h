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

#ifndef ENCODER_MP3_H
#define ENCODER_MP3_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <pthread.h>
#include <functional>
#include <mutex>

#include "Encoder.h"

namespace core
{

class EncoderMP3 : public Encoder
{
public:
    EncoderMP3(common::AudioFormatType input_type,
               uint16_t thread_number = 1,
               bool verbose = false);
    virtual ~EncoderMP3();

    const std::string& get_encoder_version() const;

    virtual common::ErrorCode start_encoding() override;
    virtual common::ErrorCode cancel_encoding() override;

private:
    struct EncoderThreadArg
    {
        uint32_t thread_id;
        std::map<std::string, bool>* input_files;
        std::atomic<bool>* cancelled;
        std::function<void(const std::string&, const std::string&)> callback;
        bool verbose;
        std::string output_directory;
    };

    static void* processing_files(void* arg);
    void on_encoding_status(const std::string& key, const std::string& value);
    static std::string get_flac_version();
};

} // core

#endif // ENCODER_MP3_H
