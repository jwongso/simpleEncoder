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

#ifndef MP3_HEADER_H
#define MP3_HEADER_H

#include <stdint.h>
#include <vector>
#include <string>

namespace utils
{

struct ID3Tag
{
    char id3[ 3 ];                          /// "ID3" id (3 bytes)
    char version[ 2 ];                      /// Version major and revision (2 bytes)
    bool flags[ 4 ];                        /// Flags (1 byte)
    char offset;                            /// Offset (1 byte)
    uint32_t extended_size;                 /// Extended header size
    std::vector< std::string > frames[ 2 ]; /// Frames
};

struct Mp3Header
{
    char ff[ 1 ];                       /// "FF" (1 bytes)
    float mpeg_version;                 /// mpeg version (1 bytes)
    uint32_t layer;                     /// Layer
    bool crc;                           /// CRC
    bool info[ 3 ];                     /// Info
    uint32_t emphasis;                  /// Emphasis
    uint32_t sampling_rate;             /// Sampling rate
    uint32_t channel_mode;              /// Channel mode
    uint32_t mode_extension[ 2 ];       /// Channel mode extension
    bool padding;                       /// Padding
    uint32_t bit_rate;                  /// Bit rate
    uint32_t frame_size;                /// Frame size
};

} // utils

#endif // MP3_HEADER_H
