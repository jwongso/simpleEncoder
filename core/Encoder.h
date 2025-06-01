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
#include <map>
#include <algorithm>
#include <mutex>
#include <atomic>

#include "common/Common.h"
#include "utils/FileSystemHelper.h"
#include "utils/WaveHeader.h"
#include "utils/WaveFileWrapper.h"

/**
 * @brief Global mutex for thread synchronization during file processing.
 * 
 * This mutex ensures thread-safe access to shared resources during multi-threaded
 * encoding operations. It protects critical sections where multiple threads might
 * access or modify shared data structures like the file processing queue.
 */
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @class Encoder
 * @brief Abstract base class for audio encoding operations.
 *
 * This class provides a framework for scanning input directories, managing multi-threaded
 * encoding of audio files, and handling output. It supports cancellation and status reporting.
 * Derived classes must implement format-specific encoding logic (e.g., via process_single_file()).
 * Thread-safety is ensured for shared resources like file lists.
 *
 * @note This class is not intended for direct instantiation; use EncoderFactory instead.
 * @see EncoderFactory
 */
namespace core
{
class Encoder
{
public:
    /**
     * @brief Deleted default constructor to prevent direct instantiation.
     */
    Encoder() = delete;

    /**
     * @brief Virtual destructor for proper cleanup in derived classes.
     */
    virtual ~Encoder()
    {}

    /**
     * @brief Scans the input directory for valid audio files based on the input type.
     * @param dir The path to the input directory to scan.
     * @return common::ErrorCode::ERROR_NONE on success, or an error code (e.g., ERROR_NOT_FOUND if the directory doesn't exist or no valid files are found).
     * @note For WAV input, files are validated using WaveFileWrapper. Invalid files are filtered out.
     */
    common::ErrorCode scan_input_directory(const std::string& dir)
    {
        utils::FileSystemHelper fs;
        if ( !fs.directory_exists( dir ) )
        {
            return common::ErrorCode::ERROR_NOT_FOUND;
        }

        std::vector< std::string > files;

        if ( !fs.get_file_paths( dir, files ) )
        {
            return common::ErrorCode::ERROR_NOT_FOUND;
        }

        if ( m_input_type == common::AudioFormatType::WAV )
        {
            files.erase( std::remove_if( files.begin( ), files.end( ),
                                        [ & ] ( const std::string& filename )
            {
                utils::WaveHeader header;
                return ( !utils::WaveFileWrapper::validate( filename, header ) );
            } ), files.end( ) );
        }

        m_input_files = files;

        return common::ErrorCode::ERROR_NONE;
    }

    /**
     * @brief Sets the output directory for encoded files.
     * @param dir The path to the output directory.
     * @note Ensure the directory exists and is writable; no validation is performed here.
     */
    void set_output_directory(const std::string& dir)
    {
        m_output_directory = dir;
    }

    /**
     * @brief Retrieves the currently set output directory.
     * @return A const reference to the output directory string.
     */
    const std::string& get_output_directory() const
    {
        return m_output_directory;
    }

    /**
     * @brief Retrieves the list of input files discovered during scanning.
     * @return A const reference to the vector of input file paths.
     */
    const std::vector<std::string>& get_input_files() const
    {
        return m_input_files;
    }

    /**
     * @brief Pure virtual method to get the version of the encoder.
     * @return A const reference to the encoder version string.
     * @note Must be implemented by derived classes to provide format-specific version info.
     */
    virtual const std::string& get_encoder_version() const = 0;

    /**
     * @brief Starts the multi-threaded encoding process for all input files.
     * @return common::ErrorCode::ERROR_NONE on success, or an error code (e.g., ERROR_NOT_FOUND if no files, ERROR_PTHREAD_CREATE on thread failure).
     * @note Creates up to m_thread_number threads to process files concurrently. Each thread processes files until none remain or cancellation occurs.
     * @warning Blocking call; waits for all threads to complete.
     * @see cancel_encoding()
     */
    common::ErrorCode start_encoding() {
        if (m_input_files.empty()) {
            return common::ErrorCode::ERROR_NOT_FOUND;
        }

        m_cancelled = false;
        m_to_be_encoded_files.clear();
        for (const auto& file : m_input_files) {
            m_to_be_encoded_files[file] = false;
        }

        std::vector<pthread_t> threads(m_thread_number);
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

        // Thread arguments
        struct ThreadTask {
            Encoder* encoder;
            std::string output_dir;
            uint32_t thread_id;
        };

        auto worker = [](void* arg) -> void* {
            ThreadTask* task = static_cast<ThreadTask*>(arg);
            bool found_file = true;  // Initialize to enter the loop

            // Loop to process multiple files per thread until none remain or cancelled
            while (!task->encoder->m_cancelled && found_file) {
                std::string input_file;
                found_file = false;  // Reset for this iteration

                // Thread-safe file acquisition
                pthread_mutex_lock(&process_mutex);
                for (auto& [file, processed] : task->encoder->m_to_be_encoded_files) {
                    if (!processed) {
                        input_file = file;
                        processed = true;
                        found_file = true;
                        break;  // Process one file at a time, then loop again
                    }
                }
                pthread_mutex_unlock(&process_mutex);

                // If a file was found, process it
                if (found_file && !input_file.empty()) {
                    task->encoder->process_single_file(
                        input_file,
                        task->output_dir,
                        task->thread_id,
                        [task](auto&& k, auto&& v) {
                            task->encoder->on_encoding_status(k, v);
                        }
                    );
                }
            }
            return nullptr;
        };

        std::vector<ThreadTask> tasks(m_thread_number);
        for (uint32_t i = 0; i < m_thread_number; ++i) {
            tasks[i] = {
                .encoder = this,
                .output_dir = m_output_directory,
                .thread_id = i + 1
            };

            if (pthread_create(&threads[i], &thread_attr, worker, &tasks[i]) != 0) {
                return common::ErrorCode::ERROR_PTHREAD_CREATE;
            }
        }

        pthread_attr_destroy(&thread_attr);
        for (auto& thread : threads) {
            pthread_join(thread, nullptr);
        }

        return common::ErrorCode::ERROR_NONE;
    }

    /**
     * @brief Cancels the ongoing encoding process.
     * @return common::ErrorCode::ERROR_CANCELLED to indicate cancellation.
     * @note Sets a flag to stop new file processing; does not interrupt ongoing encodings.
     */
    virtual common::ErrorCode cancel_encoding()
    {
        m_cancelled = true;
        return common::ErrorCode::ERROR_CANCELLED;
    }

protected:
    /**
     * @brief Protected constructor for derived classes.
     * @param input_type The format of input audio files (e.g., WAV).
     * @param output_type The target output format for encoding.
     * @param encoder_version Optional version string for the encoder.
     * @param thread_number Number of threads to use for parallel encoding (default: 1).
     * @param cancelled Initial cancellation state (default: false).
     * @param verbose If true, enables verbose logging (default: false).
     */
    Encoder(common::AudioFormatType input_type,
            common::AudioFormatType output_type,
            const std::string& encoder_version = "",
            uint16_t thread_number = 1,
            bool cancelled = false,
            bool verbose = false)
    : m_input_type( input_type )
    , m_output_type( output_type )
    , m_encoder_version(encoder_version)
    , m_thread_number(thread_number)
    , m_cancelled(cancelled)
    , m_verbose(verbose)
    {}

    /**
     * @struct EncoderThreadArg
     * @brief Structure to pass arguments to encoder threads
     */
    struct EncoderThreadArg
    {
        uint32_t thread_id;
        std::map<std::string, bool>* input_files;
        std::atomic<bool>* cancelled;
        std::function<void(const std::string&, const std::string&)> callback;
        bool verbose;
        std::string output_directory;
        Encoder* encoder;  // Pointer to the encoder instance
    };

    /**
     * @brief Pure virtual method to process a single input file.
     * @param input_file Path to the input audio file.
     * @param output_dir Directory to save the encoded file.
     * @param thread_id ID of the thread performing the encoding.
     * @param status_cb Callback function for reporting encoding status.
     * @return common::ErrorCode indicating success or failure.
     * @note Must be implemented by derived classes for format-specific encoding.
     */
    virtual common::ErrorCode process_single_file(
        const std::string& input_file,
        const std::string& output_dir,
        uint32_t thread_id,
        const std::function<void(const std::string&, const std::string&)>& status_cb) = 0;

    /**
     * @brief Callback handler for encoding status updates.
     * @param key Status key (e.g., file name or progress indicator).
     * @param value Associated status value.
     * @note Thread-safe; logs status and optionally prints if verbose mode is enabled.
     */
    void on_encoding_status(const std::string& key, const std::string& value)
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        std::string log = key + " " + value;
        m_status.emplace_back(log);

        if (m_verbose)
        {
            std::cout << log << std::endl;
        }
    }

protected:
    /// @brief Input audio format type (e.g., WAV).
    common::AudioFormatType m_input_type;
    /// @brief Output audio format type (e.g., MP3).
    common::AudioFormatType m_output_type;
    /// @brief Path to the input directory.
    std::string m_input_directory;
    /// @brief Path to the output directory for encoded files.
    std::string m_output_directory;
    /// @brief List of discovered input file paths.
    std::vector<std::string> m_input_files;
    /// @brief Map of files to encode, with processing status (thread-safe).
    std::map< std::string, bool > m_to_be_encoded_files;
    /// @brief List of status messages from encoding.
    std::vector<std::string> m_status;
    /// @brief Version string of the encoder.
    std::string m_encoder_version;
    /// @brief Number of threads for parallel encoding.
    uint16_t m_thread_number;
    /// @brief Atomic flag for cancellation status.
    std::atomic<bool> m_cancelled;
    /// @brief Mutex for thread-safe operations (e.g., status updates).
    std::mutex m_mutex;
    /// @brief If true, enables verbose output during encoding.
    bool m_verbose;
};

} // core

#endif // ENCODER_H
