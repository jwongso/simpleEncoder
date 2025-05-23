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

#include <iostream>
#include <map>
#include <cstring>
#include <thread>
#include <memory>

#include <CLI.hpp>
#include "core/EncoderFactory.h"
#include "utils/FileSystemHelper.h"
#include "utils/Helper.h"

// -------------------------------------------------------------------------------------------------

const char*
error_to_string(common::ErrorCode error)
{
    static const std::map<common::ErrorCode, const char*> s_error_strings =
    {
        { common::ErrorCode::ERROR_NONE, "Error none" },
        { common::ErrorCode::ERROR_NOT_FOUND, "Not found" },
        { common::ErrorCode::ERROR_NOT_IMPLEMENTED, "Not implemented" },
        { common::ErrorCode::ERROR_PTHREAD_CREATE, "pthread create error" },
        { common::ErrorCode::ERROR_PTHREAD_JOIN, "pthread join error" },
        { common::ErrorCode::ERROR_BUSY, "pthread error" }
    };

    auto found = s_error_strings.find(error);

    if (found == s_error_strings.end())
    {
        return "Unknown error";
    }

    return found->second;
}

// -------------------------------------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    CLI::App app{"Audio Encoder - Convert WAV files to various formats"};

    // Required parameters
    std::string input_dir;
    app.add_option("-i,--input", input_dir, "Input directory containing WAV files")
        ->required()
        ->check(CLI::ExistingDirectory);

    std::string output_dir;
    app.add_option("-o,--output", output_dir, "Output directory for encoded files")
        ->required();

    // Optional parameters
    std::string format = "flac";
    auto supported_formats = core::EncoderFactory::get_supported_formats();
    app.add_option("-f,--format", format, "Output format")
        ->check(CLI::IsMember(supported_formats))
        ->default_str("flac");

    uint16_t threads = 1;
    uint16_t max_threads = std::thread::hardware_concurrency();
    app.add_option("-j,--threads", threads, "Number of encoding threads")
        ->default_val(max_threads > 2 ? max_threads / 2 : 1)
        ->check(CLI::Range(1, static_cast<int>(max_threads)));

    // Todo: Enable fprintf, std::cout in all derived encoder classes.
    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Enable verbose output")
        ->default_val(false);

    // Parse command line arguments
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // Validate input directory
    std::string canonical_input_path;
    if (!utils::FileSystemHelper::canonical_path(input_dir, canonical_input_path)) {
        std::cerr << "The given input directory: " << input_dir << " is not valid!" << std::endl;
        return 1;
    }

    // Create output directory if it doesn't exist
    if (!utils::FileSystemHelper::ensure_directory_exists(output_dir)) {
        std::cerr << "Failed to create output directory: " << output_dir << std::endl;
        return 1;
    }

    // Create encoder based on format
    common::AudioFormatType output_format = core::EncoderFactory::string_to_format_type(format);
    auto encoder = core::EncoderFactory::create_encoder(output_format, common::AudioFormatType::WAV, threads, verbose);

    if (!encoder) {
        std::cerr << "Failed to create encoder for format: " << format << std::endl;
        return 1;
    }

    // Set encoder options
    encoder->set_output_directory(output_dir);

    // Scan input directory
    auto error = encoder->scan_input_directory(canonical_input_path);
    if (error != common::ErrorCode::ERROR_NONE) {
        std::cerr << "Error while scanning the input directory: " <<
                    error_to_string(error) << std::endl;
        return 1;
    }

    const auto& wav_files = encoder->get_input_files();
    if (wav_files.empty()) {
        std::cerr << "No valid WAV files found in the input directory." << std::endl;
        return 0;
    }

    // Display information
    std::cout << "Found " << wav_files.size() <<
                " valid WAV files to be encoded using " <<
                encoder->get_encoder_version() << ":" << std::endl;

    if (verbose) {
        for (const auto& wav : wav_files) {
            std::cout << wav << std::endl;
        }
    } else {
        std::cout << "Use --verbose to see the list of files" << std::endl;
    }

    // Start encoding
    std::cout << "Starting encoding with " << threads << " threads..." << std::endl;
    error = encoder->start_encoding();

    if (error != common::ErrorCode::ERROR_NONE) {
        std::cerr << "Error during encoding: " << error_to_string(error) << std::endl;
        return 1;
    }

    std::cout << "Encoding completed successfully!" << std::endl;
    return 0;
}

// -------------------------------------------------------------------------------------------------
