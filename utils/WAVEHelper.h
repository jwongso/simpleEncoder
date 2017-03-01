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

#ifndef SIMPLE_WAVE_VALIDATOR_H
#define SIMPLE_WAVE_VALIDATOR_H

#include <string>
#include <vector>
#include <stdint.h>

namespace utils
{

class WAVEHelper
{
public:

    /// Total header site = 44 bytes.
    struct WAVEHeader
    {
        char riff[ 4 ];                     /// "RIFF" (4 bytes)
        uint32_t file_length;               /// file length
        char wave[ 4 ];                     /// "WAVE" (4 bytes)
        char fmt[ 4 ];                      /// "fmt " (4 bytes)
        uint32_t chunk_size;                /// should be 16 or 18
        uint16_t format;                    /// 0x01 for PCM - other modes unsupported
        uint16_t channels;                  /// number of channels (1 mono, 2 stereo)
        uint32_t sampes_per_sec;            /// e.g. 44100
        uint32_t bytes_per_sec;             /// e.g. 4*44100
        uint16_t block_align;               /// bytes per sample (all channels, e.g. 4)
        uint16_t bits_per_sample;           /// bits per sample and channel, e.g. 16
        char data[ 4 ];                     /// "data" (4 bytes)
        uint32_t data_size;                 /// data size
    };


public:

    static bool validate( const std::string& filename );

    static bool get_wave_header( const std::string& filename,
                                 WAVEHeader& header_data,
                                 uint32_t& last_pos );

    static bool get_wave_data( const std::string& filename,
                               WAVEHeader& header_data,
                               int16_t*& left,
                               int16_t*& right );
};

} // utils

#endif // SIMPLE_WAVE_VALIDATOR_H
