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

#ifndef WAVE_FILE_WRAPPER_H
#define WAVE_FILE_WRAPPER_H

#include <string>
#include <vector>

#include "WaveHeader.h"

namespace utils
{

class WaveFileWrapper
{

public:

    WaveFileWrapper( ) = delete;

    WaveFileWrapper( const std::string& filename );

    bool is_valid( ) const;

    bool get_wave_data( WaveHeader& header_data, int16_t*& left, int16_t*& right ) const;

    // Fast way to validate whether a given filename is a WAVE file or not.
    static bool validate( const std::string& filename, WaveHeader& header );

private:

    const std::string& m_filename;

    WaveHeader m_header;

    bool m_valid;

    uint32_t m_last_header_pos;
};

} // utils

#endif // WAVE_FILE_WRAPPER_H
