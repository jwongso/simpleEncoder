#ifndef ENCODER_AAC_H
#define ENCODER_AAC_H

#include "Encoder.h"
#include "utils/WaveFileWrapper.h"
#include "utils/Helper.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <functional>
#include <vector>
#include <map>

const std::string FFMPEG_AAC = "FFmpeg AAC ";
const std::string AAC_EXT = ".aac";

std::string get_encoder_name() {
    const AVCodec* codec = avcodec_find_encoder_by_name("libfdk_aac");
    return (codec) ? "libfdk_aac" : "FFmpeg AAC";
}

namespace core
{

class EncoderAAC : public Encoder
{
public:
    EncoderAAC(common::AudioFormatType input_type, uint16_t thread_number, bool verbose)
    : Encoder(input_type,
           common::AudioFormatType::AAC,
           get_encoder_name() + " (FFmpeg " + get_ffmpeg_version() + ")",
           thread_number,
           false,
           verbose)
    {
    }

    virtual ~EncoderAAC() {}

    const std::string& get_encoder_version() const override
    {
        return m_encoder_version;
    }

private:
    std::string get_ffmpeg_version() const
    {
        // Updated to use modern FFmpeg version reporting
        return std::to_string(avformat_version() >> 16) + "." +
               std::to_string((avformat_version() >> 8) & 0xFF) + "." +
               std::to_string(avformat_version() & 0xFF);
    }

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
                fprintf(stderr, "[AAC] Error: Invalid WAV file '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "Invalid WAV file: " + input_file);
            return common::ErrorCode::ERROR_WAV_INVALID;
        }

        size_t last_slash = input_file.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos)
                            ? input_file
                            : input_file.substr(last_slash + 1);
        std::string output_file = output_dir + "/" +
                                utils::Helper::generate_output_file(filename, AAC_EXT);

        utils::WaveHeader header;
        int16_t* left = nullptr;
        int16_t* right = nullptr;

        if (!wave.get_wave_data(header, left, right)) {
            if (m_verbose) {
                fprintf(stderr, "[AAC] Error: Failed to read PCM data from '%s'.\n", input_file.c_str());
            }
            status_cb("Error", "Failed to read PCM data from WAV file: " + input_file);
            return common::ErrorCode::ERROR_READ_FILE;
        }

        // Initialize FFmpeg components
        AVFormatContext* output_format_context = nullptr;
        AVCodecContext* codec_context = nullptr;
        AVStream* stream = nullptr;
        const AVCodec* codec = nullptr;
        SwrContext* swr_context = nullptr;
        AVFrame* frame = nullptr;
        AVPacket* packet = nullptr;

        // Prefer libfdk_aac if available, fall back to built-in AAC
        codec = avcodec_find_encoder_by_name("libfdk_aac");
        if (!codec) {
            if (m_verbose) {
                fprintf(stderr, "[AAC] Warning: libfdk_aac not found, falling back to built-in AAC encoder.\n");
            }
            codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        }
        if (!codec) {
            if (m_verbose) {
                fprintf(stderr, "[AAC] Error: AAC encoder not found.\n");
            }
            status_cb("Error", "AAC encoder not found");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_NOT_IMPLEMENTED;
        }

        if (codec && m_verbose) {
            fprintf(stderr, "[AAC] Using encoder: %s\n", codec->name);
        }

        // Allocate format context
        if (avformat_alloc_output_context2(&output_format_context, nullptr, "adts", output_file.c_str()) < 0) {
            status_cb("Error", "Failed to allocate output context");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Create stream
        stream = avformat_new_stream(output_format_context, nullptr);
        if (!stream) {
            status_cb("Error", "Failed to create output stream");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Allocate codec context
        codec_context = avcodec_alloc_context3(codec);
        if (!codec_context) {
            status_cb("Error", "Failed to allocate codec context");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Set codec parameters (updated for modern FFmpeg and libfdk_aac compatibility)
        codec_context->sample_rate = header.sample_rate;
        codec_context->ch_layout = AV_CHANNEL_LAYOUT_STEREO;  // Updated to use modern channel layout
        codec_context->bit_rate = 128000;  // 128 kbps
        if (codec && std::string(codec->name) == "libfdk_aac") {
            codec_context->sample_fmt = AV_SAMPLE_FMT_S16; // libfdk_aac requires S16
        } else {
            codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP; // Native FFmpeg AAC prefers FLTP
        }

        // Open codec
        if (avcodec_open2(codec_context, codec, nullptr) < 0) {
            status_cb("Error", "Failed to open codec");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Copy codec parameters to stream
        if (avcodec_parameters_from_context(stream->codecpar, codec_context) < 0) {
            status_cb("Error", "Failed to copy codec parameters");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Open output file
        if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&output_format_context->pb, output_file.c_str(), AVIO_FLAG_WRITE) < 0) {
                status_cb("Error", "Failed to open output file");
                cleanup(output_format_context, codec_context, swr_context, frame, packet);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }
        }

        // Write header
        if (avformat_write_header(output_format_context, nullptr) < 0) {
            status_cb("Error", "Failed to write header");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Initialize resampler if needed
        bool needs_resample = (header.bits_per_sample != 16) ||
                             (codec_context->sample_fmt != AV_SAMPLE_FMT_S16 && codec_context->sample_fmt != AV_SAMPLE_FMT_FLTP);

        if (needs_resample) {
            swr_context = swr_alloc();
            if (!swr_context) {
                status_cb("Error", "Failed to allocate resampler context");
                cleanup(output_format_context, codec_context, swr_context, frame, packet);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }

            // Set resampler parameters (updated for modern FFmpeg)
            av_opt_set_chlayout(swr_context, "in_chlayout", &codec_context->ch_layout, 0);
            av_opt_set_chlayout(swr_context, "out_chlayout", &codec_context->ch_layout, 0);
            av_opt_set_int(swr_context, "in_sample_rate", header.sample_rate, 0);
            av_opt_set_int(swr_context, "out_sample_rate", header.sample_rate, 0);
            av_opt_set_sample_fmt(swr_context, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
            av_opt_set_sample_fmt(swr_context, "out_sample_fmt", codec_context->sample_fmt, 0);

            if (swr_init(swr_context) < 0) {
                status_cb("Error", "Failed to initialize resampler");
                cleanup(output_format_context, codec_context, swr_context, frame, packet);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }
        }

        // Allocate frame and packet
        frame = av_frame_alloc();
        packet = av_packet_alloc();
        if (!frame || !packet) {
            status_cb("Error", "Failed to allocate frame or packet");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Set frame parameters
        frame->nb_samples = codec_context->frame_size;
        frame->format = codec_context->sample_fmt;
        frame->ch_layout = codec_context->ch_layout;
        frame->sample_rate = codec_context->sample_rate;

        if (av_frame_get_buffer(frame, 0) < 0) {
            status_cb("Error", "Failed to allocate frame buffer");
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Prepare input buffer
        uint32_t samples = header.data_size / header.block_align;
        int16_t* input_buffer = new int16_t[samples * header.channels];

        // Interleave left and right channels if stereo
        if (header.channels == 2) {
            for (uint32_t i = 0; i < samples; i++) {
                input_buffer[i * 2] = left[i];
                input_buffer[i * 2 + 1] = right[i];
            }
        } else {
            memcpy(input_buffer, left, samples * sizeof(int16_t));
        }

        // Process audio in chunks (simplified loop)
        int64_t pts = 0;
        uint32_t position = 0;

        while (position < samples) {
            uint32_t remaining = samples - position;
            uint32_t chunk_size = std::min(remaining, (uint32_t)codec_context->frame_size);

            // Fill frame
            if (needs_resample) {
                // Resample if necessary
                const uint8_t* in_data[] = { (const uint8_t*)(input_buffer + position * header.channels) };
                if (swr_convert(swr_context, frame->data, chunk_size, in_data, chunk_size) < 0) {
                    status_cb("Error", "Failed to convert samples");
                    delete[] input_buffer;
                    cleanup(output_format_context, codec_context, swr_context, frame, packet);
                    delete[] left;
                    delete[] right;
                    return common::ErrorCode::ERROR_IO;
                }
            } else {
                // Direct copy for compatible formats
                memcpy(frame->data[0], input_buffer + position * header.channels,
                       chunk_size * header.channels * sizeof(int16_t));
            }

            // Set frame PTS
            frame->pts = pts;
            pts += frame->nb_samples;

            // Encode frame
            if (avcodec_send_frame(codec_context, frame) < 0) {
                status_cb("Error", "Failed to send frame to encoder");
                delete[] input_buffer;
                cleanup(output_format_context, codec_context, swr_context, frame, packet);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }

            // Receive and write packets
            while (true) {
                int ret = avcodec_receive_packet(codec_context, packet);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    status_cb("Error", "Failed to receive packet from encoder");
                    delete[] input_buffer;
                    cleanup(output_format_context, codec_context, swr_context, frame, packet);
                    delete[] left;
                    delete[] right;
                    return common::ErrorCode::ERROR_IO;
                }

                // Write packet
                av_packet_rescale_ts(packet, codec_context->time_base, stream->time_base);
                packet->stream_index = stream->index;

                if (av_interleaved_write_frame(output_format_context, packet) < 0) {
                    status_cb("Error", "Failed to write packet");
                    delete[] input_buffer;
                    cleanup(output_format_context, codec_context, swr_context, frame, packet);
                    delete[] left;
                    delete[] right;
                    return common::ErrorCode::ERROR_IO;
                }

                av_packet_unref(packet);
            }

            position += chunk_size;
        }

        // Flush encoder
        if (avcodec_send_frame(codec_context, nullptr) < 0) {
            status_cb("Error", "Failed to flush encoder");
            delete[] input_buffer;
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Receive remaining packets
        while (true) {
            int ret = avcodec_receive_packet(codec_context, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                status_cb("Error", "Failed to receive remaining packets");
                delete[] input_buffer;
                cleanup(output_format_context, codec_context, swr_context, frame, packet);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }

            av_packet_rescale_ts(packet, codec_context->time_base, stream->time_base);
            packet->stream_index = stream->index;

            if (av_interleaved_write_frame(output_format_context, packet) < 0) {
                status_cb("Error", "Failed to write remaining packets");
                delete[] input_buffer;
                cleanup(output_format_context, codec_context, swr_context, frame, packet);
                delete[] left;
                delete[] right;
                return common::ErrorCode::ERROR_IO;
            }

            av_packet_unref(packet);
        }

        // Write trailer
        if (av_write_trailer(output_format_context) < 0) {
            status_cb("Error", "Failed to write trailer");
            delete[] input_buffer;
            cleanup(output_format_context, codec_context, swr_context, frame, packet);
            delete[] left;
            delete[] right;
            return common::ErrorCode::ERROR_IO;
        }

        // Clean up
        delete[] input_buffer;
        cleanup(output_format_context, codec_context, swr_context, frame, packet);
        delete[] left;
        delete[] right;

        status_cb("Complete", "Output: " + output_file);
        return common::ErrorCode::ERROR_NONE;
    }

    void cleanup(AVFormatContext* format_context, AVCodecContext* codec_context,
                SwrContext* swr_context, AVFrame* frame, AVPacket* packet) {
        // Close output file
        if (format_context && !(format_context->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_context->pb);
        }

        // Free resources (updated for modern FFmpeg)
        av_packet_free(&packet);
        av_frame_free(&frame);
        swr_free(&swr_context);
        avcodec_free_context(&codec_context);
        avformat_free_context(format_context);
    }
};

} // core

#endif // ENCODER_AAC_H
