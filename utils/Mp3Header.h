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
#include <map>
#include <string>

namespace utils
{

struct ID3Tag
{
    char id3[ 3 ];                                  /// "ID3" id (3 bytes)
    uint8_t version_major;                          /// Version major (1 byte)
    uint8_t version_revision;                       /// Version revision (1 byte)
    bool flags[ 4 ];                                /// Flags (1 byte)
    uint32_t offset;                                /// Offset (1 byte)
    uint32_t extended_size;                         /// Extended header size
    std::map< std::string, std::string > frames;    /// Frames
};

struct Mp3Header
{
    float mpeg_version;                 /// mpeg version (1 byte)
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
