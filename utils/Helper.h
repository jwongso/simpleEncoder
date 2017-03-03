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

#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>
#include <vector>
#include <string>
#include <functional>

namespace utils
{

class Helper
{
public:

    static void read_as_chars( const std::vector< uint8_t >& input,
                               uint32_t pos,
                               uint32_t length,
                               char* target );

    static uint32_t read_as_uint32_big( const std::vector< uint8_t >& input, uint32_t pos );

    static uint32_t read_as_uint32_little( const std::vector< uint8_t >& input, uint32_t pos );

    static uint16_t read_as_uint16( const std::vector< uint8_t >& input, uint32_t pos );

    static std::string generate_output_file( const std::string& input_file,
                                             const std::string& extension );

    static void log( const std::function<
                     void( const std::string&, const std::string& ) >& callback,
                     uint32_t id,
                     const std::string& status );
};

} // utils

#endif // HELPER_H
