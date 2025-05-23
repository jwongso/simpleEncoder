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

#ifndef ENCODER_OPUS_H
#define ENCODER_OPUS_H

#include "Encoder.h"
#include <mutex>
#include <functional>
#include <vector>
#include <map>

namespace core
{

class EncoderOpus : public Encoder
{
public:
    EncoderOpus( common::AudioFormatType input_type,
                 uint16_t thread_number = 1,
                 bool verbose = false);
    virtual ~EncoderOpus( );

    const std::string& get_encoder_version( ) const;

    virtual common::ErrorCode start_encoding( ) override;
    virtual common::ErrorCode cancel_encoding( ) override;

private:
    struct EncoderThreadArg
    {
        uint32_t thread_id;
        std::map< std::string, bool >* input_files;
        std::atomic<bool>* cancelled;
        std::function< void( const std::string&, const std::string& ) > callback;
        bool verbose;
        std::string output_directory;
    };

    static void* processing_files( void* arg );
    void on_encoding_status( const std::string& key, const std::string& value );
    static std::string get_opus_version( );
};

} // core

#endif // ENCODER_OPUS_H
