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

#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <vector>

#include "common/ErrorCodes.h"

namespace core
{

class Encoder
{

public:

    // Enum for input uncompressed audio format.
    // This will be used to scan file(s) extension based on input type.
    enum class InputAudioType
    {
        INPUT_UNKNOWN = 0,
        INPUT_WAV,
        INPUT_AIFF
    };

    // Enum for output/target lossy compressed audio format.
    // This will be used to generate target file(s) extension based on output type.
    enum class OutputEncoderType
    {
        OUTPUT_UNKNOWN = 0,
        OUTPUT_MP3,
        OUTPUT_AAC,
        OUTPUT_VORBIS,
        OUTPUT_WMA
    };

public:

    Encoder( ) = delete;

    virtual ~Encoder( );

    common::ErrorCode scan_input_directory( const std::string& dir );

    const std::vector< std::string >& get_input_files( ) const;

    virtual common::ErrorCode start_encoding( ) = 0;

    virtual common::ErrorCode cancel_encoding( ) = 0;

protected:

    Encoder( InputAudioType input_type, OutputEncoderType output_type );

protected:

    InputAudioType m_input_type;
    OutputEncoderType m_output_type;
    std::string m_input_directory;
    std::vector< std::string > m_input_files;
};

} // core

#endif // ENCODER_H
