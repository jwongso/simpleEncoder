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

/**
 * @brief Macro for error checking and early return with logging.
 * 
 * This macro executes a function and checks its return value. If the function
 * returns a non-zero value (indicating an error), it logs the error with
 * file and line information, then returns the specified error code.
 * 
 * @param func The function call to execute and check
 * @param err The error code to return if the function fails
 * 
 * Usage example:
 * @code
 * RETURN_ERR_IF_ERROR(some_function(), common::ErrorCode::ERROR_IO);
 * @endcode
 */
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
 * @enum AudioFormatType
 * @brief Enumeration of supported audio format types for input and output.
 * 
 * This enum defines the various audio formats that the encoder can handle.
 * Formats are categorized into uncompressed, lossy compressed, and lossless compressed.
 * 
 * @note Not all formats may be supported for both input and output operations.
 *       Currently, WAV is the primary input format, while others are output formats.
 */
enum class AudioFormatType
{
    UNKNOWN = 0,        ///< Unknown or unsupported format
    
    // Uncompressed formats
    WAV,                ///< Waveform Audio File Format (primary input format)
    AIFF,               ///< Audio Interchange File Format
    
    // Lossy compressed formats
    MP3,                ///< MPEG-1 Audio Layer III (using LAME encoder)
    AAC,                ///< Advanced Audio Coding (using FFmpeg)
    VORBIS,             ///< OGG Vorbis (using libvorbis)
    WMA,                ///< Windows Media Audio
    OPUS,               ///< Opus codec (using libopus)
    
    // Lossless compressed formats
    FLAC,               ///< Free Lossless Audio Codec (using libFLAC)
    ALAC                ///< Apple Lossless Audio Codec
};

/**
 * @enum ErrorCode
 * @brief Comprehensive error codes that can be returned by encoder components.
 * 
 * This enum defines all possible error conditions that may occur during
 * audio encoding operations. Error codes are categorized by their source:
 * general errors, utility errors, and encoder-specific errors.
 * 
 * @note Error codes follow a hierarchical structure where ERROR_NONE (0)
 *       indicates success, and all other values indicate specific failure modes.
 */
enum ErrorCode
{
    // General success and common errors
    ERROR_NONE = 0,             ///< Operation completed successfully
    ERROR_NOT_FOUND,            ///< File, directory, or resource not found
    ERROR_READ_FILE,            ///< Failed to read from file
    ERROR_CANCELLED,            ///< Operation was cancelled by user
    
    // Utility-specific errors
    ERROR_WAV_INVALID,          ///< Invalid or corrupted WAV file format
    
    // Core encoder errors
    ERROR_NOT_IMPLEMENTED,      ///< Feature or format not yet implemented
    ERROR_PTHREAD_CREATE,       ///< Failed to create pthread for multi-threading
    ERROR_PTHREAD_JOIN,         ///< Failed to join pthread
    
    // Format-specific encoder errors
    ERROR_LAME,                 ///< LAME MP3 encoder library error
    ERROR_OPUS,                 ///< Opus encoder library error
    ERROR_VORBIS,               ///< Vorbis encoder library error
    ERROR_FLAC,                 ///< FLAC encoder library error
    ERROR_FFMPEG,               ///< FFmpeg library error (used for AAC)
    
    // System and I/O errors
    ERROR_BUSY,                 ///< Resource is busy or locked
    ERROR_IO                    ///< General input/output error
};

} // core

#endif // ENCODER_ERRORCODES_H
