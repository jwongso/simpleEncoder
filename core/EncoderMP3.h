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

#ifndef ENCODER_MP3_H
#define ENCODER_MP3_H

#include "Encoder.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

#include <lame/lame.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <functional>
#include <vector>
#include <map>

const std::string LAME = "Lame ";
const std::string MP3_EXT = ".mp3";

namespace core
{

/**
 * @class EncoderMP3
 * @brief MP3 encoder class that handles encoding WAV files to MP3 format.
 *
 * This class provides functionality to encode audio data from WAV format to MP3 using the LAME library.
 */
class EncoderMP3 : public Encoder
{
public:
    /**
     * @brief Constructor for EncoderMP3.
     * @param input_type The input audio format type (typically WAV).
     * @param thread_number The number of threads to use for parallel encoding.
     * @param verbose Verbose mode flag for detailed logging during encoding.
     * 
     * Initializes the MP3 encoder with LAME library version information and
     * sets up the encoder for multi-threaded operation.
     */
    EncoderMP3( common::AudioFormatType input_type, uint16_t thread_number, bool verbose )
    : Encoder( input_type,
           common::AudioFormatType::MP3,
           LAME + get_lame_version( ),
           thread_number,
           false,
           verbose)
    {
    }

    /**
     * @brief Destructor for EncoderMP3.
     */
    virtual ~EncoderMP3( ) {}

    /**
     * @brief Gets the version of the encoder.
     * @return The encoder version string.
     */
    const std::string& get_encoder_version( ) const
    {
        return m_encoder_version;
    }

private:
    /**
     * @brief Processes a single file for encoding.
     * @param input_file The path to the input WAV file.
     * @param output_dir The directory to save the output MP3 file.
     * @param thread_id The ID of the thread processing this file.
     * @param status_cb Callback function for status updates.
     * @return Error code indicating success or failure.
     */
    common::ErrorCode process_single_file(
        const std::string& input_file,
        const std::string& output_dir,
        uint32_t thread_id,
        const std::function<void(const std::string&, const std::string&)>& status_cb) override
    {
        status_cb("Thread " + std::to_string(thread_id), "Processing " + input_file);

        utils::WaveFileWrapper wave(input_file);
        if (!wave.is_valid()) {
            if (m_verbose) {
                fprintf(stderr, "[MP3] Error: Invalid WAV file '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "Invalid WAV file: " + input_file);
            return common::ErrorCode::ERROR_WAV_INVALID;
        }

        size_t last_slash = input_file.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos)
                            ? input_file
                            : input_file.substr(last_slash + 1);
        std::string output_file = output_dir + "/" +
                                utils::Helper::generate_output_file(filename, MP3_EXT);

        utils::WaveHeader header;
        int16_t* left = nullptr;
        int16_t* right = nullptr;

        if (!wave.get_wave_data(header, left, right)) {
            if (m_verbose) {
                fprintf(stderr, "[MP3] Error: Failed to read PCM data from '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "Failed to read PCM data from WAV file: " + input_file);
            return common::ErrorCode::ERROR_READ_FILE;
        }

        lame_global_flags* g_lame_flags = lame_init();
        if (!g_lame_flags) {
            if (m_verbose) {
                fprintf(stderr, "[MP3] Error: lame_init() failed for '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "LAME initialization failed for input file: " + input_file);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_LAME;
        }

        lame_set_brate(g_lame_flags, 128);
        lame_set_quality(g_lame_flags, 3);

        uint32_t samples = header.data_size / header.block_align;
        lame_set_num_channels(g_lame_flags, header.channels);
        lame_set_num_samples(g_lame_flags, samples);
        lame_set_bWriteVbrTag(g_lame_flags, 0);

        int err = lame_init_params(g_lame_flags);
        if (err != 0) {
            if (m_verbose) {
                fprintf(stderr, "[MP3] Error: lame_init_params() returned %d for '%s'.\n", err, input_file.c_str());
            }
            status_cb("Error", "Failed to initialize LAME parameters for: " + input_file);
            lame_close(g_lame_flags);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_LAME;
        }

        uint32_t buffer_size = 1.25 * samples + 7200;
        uint8_t* mp3_buffer = new uint8_t[buffer_size];

        int encoded_size = lame_encode_buffer(g_lame_flags,
                                            left,
                                            right,
                                            samples,
                                            mp3_buffer,
                                            buffer_size);

        if (encoded_size <= 0) {
            if (m_verbose) {
                fprintf(stderr, "[MP3] Error: lame_encode_buffer() returned %d for '%s'.\n", encoded_size, input_file.c_str());
            }
            status_cb("Error", "MP3 encoding failed for input file: " + input_file);
            delete[] mp3_buffer;
            lame_close(g_lame_flags);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_LAME;
        }

        FILE* output = fopen(output_file.c_str(), "wb+");
        if (!output) {
            if (m_verbose) {
                fprintf(stderr, "[MP3] Error: Failed to open output file '%s' for input '%s'.\n",
                        output_file.c_str(), input_file.c_str());
            }
            status_cb("Error", "Failed to open output file '" + output_file + "' for input: " + input_file);
            delete[] mp3_buffer;
            lame_close(g_lame_flags);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        fwrite(mp3_buffer, sizeof(uint8_t), encoded_size, output);

        uint32_t flush = lame_encode_flush(g_lame_flags, mp3_buffer, buffer_size);
        fwrite(mp3_buffer, sizeof(uint8_t), flush, output);

        lame_mp3_tags_fid(g_lame_flags, output);
        fclose(output);
        delete[] mp3_buffer;
        lame_close(g_lame_flags);
        delete[] left;
        delete[] right;

        status_cb("Complete", "Output: " + output_file);
        return common::ErrorCode::ERROR_NONE;
    }

};

} // core

#endif // ENCODER_MP3_H
