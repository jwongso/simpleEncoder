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

#ifndef AUDIOTYPE_H
#define AUDIOTYPE_H

namespace common
{

/**
 * Different audio format type for input and output
 */
enum class AudioFormatType
{
    UNKNOWN = 0,
    // Uncompressed
    WAV,
    AIFF,
    // Lossy compressed
    MP3,
    ACC,
    VORBIS,
    WMA,
    // Lossless compressed
    FLAC,
    ALAC
};

} // core

#endif // AUDIOTYPE_H
