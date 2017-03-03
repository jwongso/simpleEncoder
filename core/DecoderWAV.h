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

#ifndef DECODER_WAV_H
#define DECODER_WAV_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <pthread.h>
#include <functional>
#include <mutex>

#include "Decoder.h"

namespace core
{

class DecoderWAV : public Decoder
{
public:

    typedef std::function< void( const std::string&, const std::string& ) > Callback;

    struct DecoderThreadArg
    {
        uint32_t thread_id;
        std::map< std::string, bool >* input_files;
        bool* cancelled;
        Callback callback;
    };

public:

    DecoderWAV( common::AudioFormatType input_type, uint16_t thread_number = 1 );

    ~DecoderWAV( ) override;

    const std::string& get_decoder_version( ) const;

    common::ErrorCode start_decoding( ) override;

    common::ErrorCode cancel_decoding( ) override;

protected:

    void on_decoding_status( const std::string& key, const std::string& value );

private:

    static void* processing_files( void* arg );

private:

    std::string m_decoder_version;
    uint16_t m_thread_number;
    std::map< std::string, bool > m_to_be_decoded_files;
    bool m_cancelled;
    std::deque< std::string > m_status;
    mutable std::mutex m_mutex;
};

} // core

#endif // DECODER_WAV_H
