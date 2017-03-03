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

#ifndef MP3_FILE_WRAPPER_H
#define MP3_FILE_WRAPPER_H

#include <string>
#include <vector>

#include "Mp3Header.h"

namespace utils
{

class Mp3FileWrapper
{

public:

    Mp3FileWrapper( ) = delete;

    Mp3FileWrapper( const std::string& filename );

    bool is_valid( ) const;

    //bool get_wave_data( WaveHeader& header_data, int16_t*& left, int16_t*& right ) const;

    // Fast way to validate whether a given filename is a WAVE file or not.
    static bool validate( const std::string& filename,
                          std::vector< ID3Tag >& id3tags,
                          Mp3Header& header );

    static bool get_id3tags( const std::vector< uint8_t >& contents,
                             std::vector< ID3Tag >& id3tags,
                             uint32_t& last_pos );

    static bool get_mp3header( const std::vector< uint8_t >& contents,
                               const uint32_t offset,
                               Mp3Header& header );

private:

    const std::string& m_filename;
    std::vector< ID3Tag > m_id3tags;
    Mp3Header m_header;
    bool m_valid;
};

} // utils

#endif // MP3_FILE_WRAPPER_H
