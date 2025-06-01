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

#ifndef ENCODER_VORBIS_H
#define ENCODER_VORBIS_H

#include "Encoder.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

#include <vorbis/vorbisenc.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <map>

const std::string VORBIS = "Vorbis ";
const std::string VORBIS_EXT = ".ogg";

namespace core
{

/**
 * @class EncoderVorbis
 * @brief Vorbis OGG encoder implementation derived from the base Encoder class.
 *
 * This class provides Vorbis encoding functionality for converting WAV audio files
 * to OGG Vorbis format. It uses the libvorbis library for encoding operations.
 * The encoder supports multi-threaded processing and configurable quality settings.
 *
 * Key features:
 * - Variable bitrate (VBR) encoding with quality-based settings
 * - Support for mono and stereo audio
 * - Automatic sample rate handling (supports common rates: 8kHz to 192kHz)
 * - Thread-safe encoding operations
 * - Comprehensive error handling and status reporting
 *
 * @note This encoder requires libvorbis and libogg libraries to be installed.
 * @see Encoder base class for common functionality
 */
class EncoderVorbis : public Encoder
{
public:
    /**
     * @brief Constructor for the Vorbis encoder.
     * @param input_type The input audio format type (typically WAV).
     * @param thread_number Number of threads to use for parallel encoding.
     * @param verbose If true, enables detailed logging during encoding process.
     * 
     * Initializes the Vorbis encoder with the specified parameters and sets up
     * the encoder version string using the libvorbis version information.
     */
    EncoderVorbis(common::AudioFormatType input_type, uint16_t thread_number, bool verbose)
    : Encoder(input_type,
          common::AudioFormatType::VORBIS,
          VORBIS + get_vorbis_version(),
          thread_number,
          false,
          verbose)
    {
    }

    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~EncoderVorbis() {}

    /**
     * @brief Retrieves the encoder version string.
     * @return A const reference to the encoder version string including libvorbis version.
     */
    const std::string& get_encoder_version() const
    {
        return m_encoder_version;
    }

private:
    /**
     * @brief Processes a single WAV file and encodes it to OGG Vorbis format.
     * @param input_file Path to the input WAV file to be encoded.
     * @param output_dir Directory where the encoded OGG file will be saved.
     * @param thread_id Unique identifier for the encoding thread (for logging).
     * @param status_cb Callback function for reporting encoding progress and status.
     * @return ErrorCode indicating success (ERROR_NONE) or specific failure reason.
     *
     * This method performs the complete Vorbis encoding process:
     * 1. Validates the input WAV file
     * 2. Reads PCM audio data from the WAV file
     * 3. Initializes Vorbis encoder with appropriate settings
     * 4. Encodes audio data in chunks
     * 5. Writes the encoded OGG Vorbis stream to output file
     * 6. Handles cleanup and error reporting
     *
     * Encoding settings:
     * - Quality: 0.4 (equivalent to ~128kbps VBR)
     * - Supports sample rates from 8kHz to 192kHz
     * - Supports mono and stereo channels
     */
    virtual common::ErrorCode process_single_file(
        const std::string& input_file,
        const std::string& output_dir,
        uint32_t thread_id,
        const std::function<void(const std::string&, const std::string&)>& status_cb) override
    {
        status_cb("Thread " + std::to_string(thread_id), "Processing " + input_file);

        // Validate input WAV file
        utils::WaveFileWrapper wave(input_file);
        if (!wave.is_valid()) {
            if (m_verbose) {
                fprintf(stderr, "[Vorbis] Error: Invalid WAV file '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "Invalid WAV file: " + input_file);
            return common::ErrorCode::ERROR_WAV_INVALID;
        }

        // Generate output file path
        size_t last_slash = input_file.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos)
                            ? input_file
                            : input_file.substr(last_slash + 1);
        std::string output_file = output_dir + "/" +
                                utils::Helper::generate_output_file(filename, VORBIS_EXT);

        // Read WAV data
        utils::WaveHeader header;
        int16_t* left = nullptr;
        int16_t* right = nullptr;

        if (!wave.get_wave_data(header, left, right)) {
            if (m_verbose) {
                fprintf(stderr, "[Vorbis] Error: Failed to read PCM data from '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "Failed to read PCM data from WAV file: " + input_file);
            return common::ErrorCode::ERROR_READ_FILE;
        }

        try {
            // Initialize Vorbis encoding structures
            vorbis_info vi;
            vorbis_comment vc;
            vorbis_dsp_state vd;
            vorbis_block vb;
            ogg_stream_state os;
            ogg_page og;
            ogg_packet op;

            vorbis_info_init(&vi);

            // Setup encoding parameters (VBR mode with quality 0.4 ≈ 128kbps)
            int ret = vorbis_encode_init_vbr(&vi, header.channels, header.sample_rate, 0.4f);
            if (ret) {
                if (m_verbose) {
                    fprintf(stderr, "[Vorbis] Error: vorbis_encode_init_vbr() failed with code %d for '%s'.\n", 
                            ret, input_file.c_str());
                }
                status_cb("Error", "Failed to initialize Vorbis encoder for: " + input_file);
                vorbis_info_clear(&vi);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_VORBIS;
            }

            // Initialize comment structure
            vorbis_comment_init(&vc);
            vorbis_comment_add_tag(&vc, "ENCODER", "simpleEncoder");

            // Initialize analysis and synthesis states
            vorbis_analysis_init(&vd, &vi);
            vorbis_block_init(&vd, &vb);

            // Initialize OGG stream with random serial number
            srand(time(NULL));
            ogg_stream_init(&os, rand());

            // Create and write header packets
            ogg_packet header_main, header_comments, header_codebooks;
            vorbis_analysis_headerout(&vd, &vc, &header_main, &header_comments, &header_codebooks);
            ogg_stream_packetin(&os, &header_main);
            ogg_stream_packetin(&os, &header_comments);
            ogg_stream_packetin(&os, &header_codebooks);

            // Open output file
            FILE* output = fopen(output_file.c_str(), "wb");
            if (!output) {
                if (m_verbose) {
                    fprintf(stderr, "[Vorbis] Error: Failed to open output file '%s' for input '%s'.\n",
                            output_file.c_str(), input_file.c_str());
                }
                status_cb("Error", "Failed to open output file '" + output_file + "' for input: " + input_file);
                
                // Cleanup
                ogg_stream_clear(&os);
                vorbis_block_clear(&vb);
                vorbis_dsp_clear(&vd);
                vorbis_comment_clear(&vc);
                vorbis_info_clear(&vi);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }

            // Write header pages
            while (ogg_stream_flush(&os, &og)) {
                fwrite(og.header, 1, og.header_len, output);
                fwrite(og.body, 1, og.body_len, output);
            }

            // Prepare PCM data for encoding
            uint32_t total_samples = header.data_size / (header.channels * sizeof(int16_t));
            const int READ_SIZE = 1024; // Process in chunks of 1024 samples

            for (uint32_t i = 0; i < total_samples; i += READ_SIZE) {
                uint32_t samples_to_read = std::min(READ_SIZE, (int)(total_samples - i));
                
                // Get buffer for analysis
                float** buffer = vorbis_analysis_buffer(&vd, samples_to_read);
                
                // Convert int16 PCM to float and deinterleave
                if (header.channels == 2) {
                    for (uint32_t j = 0; j < samples_to_read; j++) {
                        buffer[0][j] = left[i + j] / 32768.0f;
                        buffer[1][j] = right[i + j] / 32768.0f;
                    }
                } else if (header.channels == 1) {
                    for (uint32_t j = 0; j < samples_to_read; j++) {
                        buffer[0][j] = left[i + j] / 32768.0f;
                    }
                }

                // Tell the library how many samples we wrote
                vorbis_analysis_wrote(&vd, samples_to_read);

                // Process analysis blocks
                while (vorbis_analysis_blockout(&vd, &vb) == 1) {
                    vorbis_analysis(&vb, NULL);
                    vorbis_bitrate_addblock(&vb);

                    while (vorbis_bitrate_flushpacket(&vd, &op)) {
                        ogg_stream_packetin(&os, &op);

                        // Write out pages
                        while (ogg_stream_pageout(&os, &og)) {
                            fwrite(og.header, 1, og.header_len, output);
                            fwrite(og.body, 1, og.body_len, output);
                        }
                    }
                }
            }

            // Signal end of stream
            vorbis_analysis_wrote(&vd, 0);

            // Flush remaining packets
            while (vorbis_analysis_blockout(&vd, &vb) == 1) {
                vorbis_analysis(&vb, NULL);
                vorbis_bitrate_addblock(&vb);

                while (vorbis_bitrate_flushpacket(&vd, &op)) {
                    ogg_stream_packetin(&os, &op);

                    while (ogg_stream_pageout(&os, &og)) {
                        fwrite(og.header, 1, og.header_len, output);
                        fwrite(og.body, 1, og.body_len, output);
                    }
                }
            }

            // Write final pages
            while (ogg_stream_flush(&os, &og)) {
                fwrite(og.header, 1, og.header_len, output);
                fwrite(og.body, 1, og.body_len, output);
            }

            // Cleanup
            fclose(output);
            ogg_stream_clear(&os);
            vorbis_block_clear(&vb);
            vorbis_dsp_clear(&vd);
            vorbis_comment_clear(&vc);
            vorbis_info_clear(&vi);
            delete[] left;
            delete[] right;

            status_cb("Complete", "Output: " + output_file);
            return common::ErrorCode::ERROR_NONE;
        }
        catch (const std::exception& e) {
            if (m_verbose) {
                fprintf(stderr, "[Vorbis] Exception: %s\n", e.what());
            }
            status_cb("Error", "Exception: " + std::string(e.what()));
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_VORBIS;
        }
    }

    /**
     * @brief Retrieves the libvorbis version string.
     * @return String containing the libvorbis version information.
     * 
     * This static method returns the version of the libvorbis library
     * being used for encoding operations.
     */
    static std::string get_vorbis_version()
    {
        return vorbis_version_string();
    }
};

} // core

#endif // ENCODER_VORBIS_H
