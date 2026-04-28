/*GRB*

    Gerbera - https://gerbera.io/

    transcode_io_handler.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

// Based in https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcode.c

/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2014 Andrey Utkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/// @file transcoding/transcode_io_handler.cc
#define GRB_LOG_FAC GrbLogFacility::transcoding

#include "transcode_io_handler.h"

#ifdef HAVE_FFMPEG

#include "config/result/transcoding.h"
#include "exceptions.h"
#include "transcoding/transcode_int_util.h"
#include "upnp/compat.h"
#include "util/logger.h"
#include "util/tools.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "util/ffmpeg_compat.h"

#include <cstring>

TranscodeInternalIOHandler::TranscodeInternalIOHandler(
    const fs::path& filename,
    fs::path tempPath,
    const std::shared_ptr<TranscodingProfile>& profile,
    const std::shared_ptr<CdsObject>& obj,
    off_t offset)
    : file(filename)
    , tmpDir(std::move(tempPath))
    , profile(profile)
    , obj(obj)
    , offset(offset)
{
    log_debug("path = {}", file.getPath().string());
}

TranscodeInternalIOHandler::~TranscodeInternalIOHandler()
{
    av_packet_free(&packet);
    avformat_close_input(&inFormatContext);
#if 0
    if (outFormatContext && outFormatContext->pb && !(outFormatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outFormatContext->pb);
#endif
    avformat_free_context(outFormatContext);
}

void TranscodeInternalIOHandler::open(enum UpnpOpenFileMode mode)
{
    log_debug("open {}", file.getPath().string());
    if (mode != UPNP_READ)
        throw_std_runtime_error("UpnpOpenFileMode {} not supported", mode);

    std::lock_guard<std::mutex> lock(mutex);
    if (open_input_file() < 0)
        throw_std_runtime_error("open input {} failed", file.getPath().string());

    outFilePath = tmpDir / fmt::format("grb-tr-{}.{}", generateRandomId(), profile->encoder.getFormat());
    if (open_output_file(outFilePath) < 0)
        throw_std_runtime_error("open output for {} failed", file.getPath().string());

    // https://www.ffmpeg.org/doxygen/8.0/group__lavf__decoding.html#ga3b40fc8d2fda6992ae6ea2567d71ba30
    // int avformat_seek_file(AVFormatContext* s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
    if (offset > 0 && avformat_seek_file(inFormatContext, -1, offset, offset, offset, AVSEEK_FLAG_BYTE) != 0)
        throw_std_runtime_error("seek to {} failed", offset);

    if (init_filters())
        throw_std_runtime_error("init filters for {} failed", file.getPath().string());

    packet = av_packet_alloc();
    if (!packet)
        throw_std_runtime_error("alloc packet for {} failed", file.getPath().string());
}

grb_read_t TranscodeInternalIOHandler::read(std::byte* buf, std::size_t length)
{
    std::lock_guard<std::mutex> lock(mutex);
    log_debug("read {} {}", file.getPath().string(), length);

    std::size_t index = 0;
    if (extraData.size() > 0) {
        std::size_t bytesToWrite = length > extraData.size() ? extraData.size() : length;
        std::memcpy(buf, extraData.data(), bytesToWrite);
        index += bytesToWrite;
        length -= bytesToWrite;
        if (length == 0) {
            if (extraData.size() > bytesToWrite)
                extraData = std::vector<std::byte>(extraData.data() + bytesToWrite + 1, extraData.data() + extraData.size());
            else
                extraData.clear();
            return bytesToWrite;
        }
    }
    if (isEOF) {
        if (index > 0) {
            return grb_read_t(index); // successful read
        }
        return GRB_READ_END;
    }

    extraData.clear();

    while (length > 0) {
        auto ret = filterPacket();
        if (ret < 0)
            return GRB_READ_ERROR;

        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            ret = closeOutput();
            if (ret < 0) {
                log_error("Error occurred: {}", av_err2str(ret));
            }
            isEOF = true;
        }
        auto buffer = readBuffer();
        std::size_t bytesToWrite = length > buffer.size() ? buffer.size() : length;
        std::memcpy(buf + index, buffer.data(), bytesToWrite);
        index += bytesToWrite;
        length -= bytesToWrite;
        if (length == 0) {
            if (buffer.size() > bytesToWrite)
                extraData = std::vector<std::byte>(buffer.data() + bytesToWrite + 1, buffer.data() + buffer.size());
            return grb_read_t(index); // successful read
        }
        if (isEOF) {
            if (index > 0) {
                return grb_read_t(index); // successful read
            }
            return GRB_READ_END;
        }
    }

    return GRB_READ_ERROR; // length is 0 without reading data
}

void TranscodeInternalIOHandler::seek(off_t offset, int whence)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!inFormatContext || !inFormatContext->pb)
        return;

    // https://www.ffmpeg.org/doxygen/8.0/avio_8h.html#a03e23bf0144030961c34e803c71f614f
    if (avio_seek(inFormatContext->pb, offset, whence) < 0) {
        throw_std_runtime_error("avio_seek failed");
    }
    extraData.clear();
}

off_t TranscodeInternalIOHandler::tell()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (inFormatContext && inFormatContext->pb)
        return avio_tell(inFormatContext->pb);
    return -1;
}

void TranscodeInternalIOHandler::close()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!outFilePath.empty())
        GrbFile(outFilePath).remove();
}

int TranscodeInternalIOHandler::open_input_file()
{
    inFormatContext = nullptr;
    log_debug("start '{}'", file.getPath().c_str());
    int ret = avformat_open_input(&inFormatContext, file.getPath().c_str(), nullptr, nullptr);
    if (ret < 0) {
        log_error("Cannot open input file {}", file.getPath().c_str());
        return ret;
    }

    ret = avformat_find_stream_info(inFormatContext, nullptr);
    if (ret < 0) {
        log_error("Cannot find stream information in {}", file.getPath().c_str());
        return ret;
    }

    for (std::size_t streamNumber = 0; streamNumber < inFormatContext->nb_streams; streamNumber++) {
        auto sCtx = std::make_shared<StreamContext>(inFormatContext->streams[streamNumber], streamNumber, inFormatContext, profile);
        if (!sCtx || !(*sCtx)) {
            log_error("Cannot create context for #{}", streamNumber);
            return AVERROR_UNKNOWN;
        }
        streamCtx.emplace(streamNumber, std::move(sCtx));
    }

    av_dump_format(inFormatContext, 0, file.getPath().c_str(), 0);
    log_debug("end '{}'", file.getPath().c_str());
    return ret;
}

int TranscodeInternalIOHandler::open_output_file(const std::string& filename)
{
    int ret;

    log_debug("start '{}'", filename);
    outFormatContext = nullptr;
    if (filename.empty())
        ret = avformat_alloc_output_context2(&outFormatContext, nullptr, profile->encoder.getFormat().c_str(), nullptr);
    else
        ret = avformat_alloc_output_context2(&outFormatContext, nullptr, profile->encoder.getFormat().c_str(), filename.c_str());
    if (ret < 0) {
        log_error("Could not create output context for '{}'", filename);
        return ret;
    }

    for (std::size_t streamNumber = 0; streamNumber < inFormatContext->nb_streams; streamNumber++) {
        AVStream* out_stream = avformat_new_stream(outFormatContext, nullptr);
        if (!out_stream) {
            log_error("Failed allocating output stream #{}", streamNumber);
            return AVERROR_UNKNOWN;
        }
        ret = streamCtx.at(streamNumber)->createEncoder(out_stream, outFormatContext);
        if (ret < 0) {
            log_error("Could not create output encoder for '{}'", filename);
            return ret;
        }
    }
    if (!filename.empty()) {
        av_dump_format(outFormatContext, 0, filename.c_str(), 1);

        if (!(outFormatContext->oformat->flags & AVFMT_NOFILE))
            ret = avio_open(&outFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            log_error("Could not open output file '{}'", filename);
            return ret;
        }

        /* init muxer, write output file header */
        ret = avformat_write_header(outFormatContext, nullptr);
        if (ret < 0) {
            log_error("Error occurred when opening output file '{}', filename");
            return ret;
        }
    } else {
        ret = avformat_init_output(outFormatContext, nullptr);
        if (ret < 0) {
            log_error("Could not open output stream");
            return ret;
        }
    }

    return ret;
}

int TranscodeInternalIOHandler::init_filters(void)
{
    log_debug("start");
    for (std::size_t i = 0; i < inFormatContext->nb_streams; i++) {
        if (!(as_codecpar(inFormatContext->streams[i])->codec_type == AVMEDIA_TYPE_AUDIO
                || as_codecpar(inFormatContext->streams[i])->codec_type == AVMEDIA_TYPE_VIDEO))
            continue;

        std::string filter_spec;
        if (as_codecpar(inFormatContext->streams[i])->codec_type == AVMEDIA_TYPE_VIDEO) {
            filter_spec = (!profile->encoder.getVFilter().empty())
                ? profile->encoder.getVFilter()
                : "null"; /* passthrough (dummy) filter for video */
        } else {
            filter_spec = (!profile->encoder.getAFilter().empty())
                ? profile->encoder.getAFilter()
                : "anull"; /* passthrough (dummy) filter for audio */
        }

        auto fCtx = std::make_shared<FilteringContext>(streamCtx.at(i)->getDecoder(), streamCtx.at(i)->getEncoder(), i, filter_spec, profile);
        filteringCtx.emplace(i, std::move(fCtx));
    }
    log_debug("end");
    return 0;
}

int TranscodeInternalIOHandler::flush_encoder(std::size_t stream_index)
{
    if (!(streamCtx.at(stream_index)->getEncoder()->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;

    log_debug("Flushing stream #{} encoder", stream_index);
    return filteringCtx.at(stream_index)->encodeWriteFrame(outFormatContext, true);
}

int TranscodeInternalIOHandler::filterPacket()
{
    int ret = av_read_frame(inFormatContext, packet);
    if (ret < 0)
        return AVERROR(EAGAIN);
    std::size_t stream_index = packet->stream_index;
    log_debug("Demuxer gave frame of stream_index #{}", stream_index);
    auto filter = filteringCtx.at(stream_index);

    if (filter && *filter) {
        auto stream = streamCtx.at(stream_index);

        log_debug("Going to reencode&filter the frame");

        ret = stream->sendPacket(packet);
        if (ret < 0) {
            log_error("Decoding failed");
            return AVERROR(EAGAIN);
        }

        while (ret >= 0) {
            ret = stream->receiveFrame();
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                break;
            else if (ret < 0)
                return ret;

            stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
            ret = filter->filterEncodeWriteFrame(stream->dec_frame, outFormatContext);
            if (ret < 0)
                return ret;
        }
    } else {
        /* remux this frame without reencoding */
        av_packet_rescale_ts(packet,
            inFormatContext->streams[stream_index]->time_base,
            outFormatContext->streams[stream_index]->time_base);

        ret = av_interleaved_write_frame(outFormatContext, packet);
        if (ret < 0)
            return ret;
    }
    av_packet_unref(packet);
    return 0;
}

int TranscodeInternalIOHandler::run(const std::string& outputFileName)
{
    int ret = open_input_file();
    if (ret < 0)
        return ret;
    ret = open_output_file(outputFileName);
    if (ret < 0)
        return ret;
    ret = init_filters();
    if (ret < 0)
        return ret;
    packet = av_packet_alloc();
    if (!packet)
        return -1;

    /* read all packets */
    while (1) {
        ret = filterPacket();
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            break;
        else if (ret < 0)
            return ret;
    }

    ret = closeOutput();

    if (ret < 0) {
        log_error("Error occurred: {}", av_err2str(ret));
    }
    return ret ? 1 : 0;
}

int TranscodeInternalIOHandler::closeOutput()
{
    int ret = 0;
    /* flush decoders, filters and encoders */
    for (std::size_t i = 0; i < inFormatContext->nb_streams; i++) {
        auto filter = filteringCtx.at(i);
        if (!filter || !*filter)
            continue;

        auto inputContext = streamCtx.at(i);

        log_debug("Flushing stream #{} decoder", i);

        /* flush decoder */
        ret = inputContext->sendPacket(nullptr);
        if (ret < 0) {
            log_error("Flushing decoding failed");
            return ret;
        }

        while (ret >= 0) {
            ret = inputContext->receiveFrame();
            if (ret == AVERROR_EOF)
                break;
            else if (ret < 0)
                return ret;

            inputContext->dec_frame->pts = inputContext->dec_frame->best_effort_timestamp;
            ret = filter->filterEncodeWriteFrame(inputContext->dec_frame, outFormatContext);
            if (ret < 0)
                return ret;
        }

        /* flush filter */
        ret = filter->filterEncodeWriteFrame(nullptr, outFormatContext);
        if (ret < 0) {
            log_error("Flushing filter failed");
            return ret;
        }

        /* flush encoder */
        ret = flush_encoder(i);
        if (ret < 0) {
            log_error("Flushing encoder failed");
            return ret;
        }
    }

    av_write_trailer(outFormatContext);
    return ret;
}

std::vector<std::byte> TranscodeInternalIOHandler::readBuffer()
{
    uint8_t* buffer = nullptr;
    std::vector<std::byte> result;
    log_debug("Fetching Result from buffer");

    if (!outFormatContext || !outFormatContext->pb)
        return result;

    if (avio_open_dyn_buf(&outFormatContext->pb) < 0) {
        log_error("Open dyn buffer failed");
        return result;
    }
    // int avio_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer)
    int size = avio_close_dyn_buf(outFormatContext->pb, &buffer);
    if (size >= 0) {
        result = std::vector<std::byte>((std::byte*)buffer, (std::byte*)buffer + size);
        log_debug("Got {} bytes from buffer", size);
        av_free(buffer);
    }
    return result;
}

#endif
