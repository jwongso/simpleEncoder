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

#ifndef ERRORCODES_H
#define ERRORCODES_H

#define RETURN_ERR_IF_ERROR( func, err ) \
do \
{ \
    int ret = ( func ); \
    if ( ret != 0 ) \
    { \
        fprintf(stderr, "Error: %s returned %d at %s:%d", #func, ret, __FILE__, __LINE__ ); \
        return err; \
    } \
} while (0)

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
    OPUS,
    // Lossless compressed
    FLAC,
    ALAC
};

/**
 * Error codes that can be returned by encoder components.
 */
enum ErrorCode
{
    ERROR_NONE = 0,
    ERROR_NOT_FOUND,
    ERROR_READ_FILE,
    ERROR_CANCELLED,
    /// utils
    ERROR_WAV_INVALID,
    /// core specific error codes
    ERROR_NOT_IMPLEMENTED,
    ERROR_PTHREAD_CREATE,
    ERROR_PTHREAD_JOIN,
    ERROR_LAME,
    ERROR_OPUS,
    ERROR_FLAC,
    ERROR_BUSY,
    ERROR_IO
};

} // core

#endif // ENCODER_ERRORCODES_H
