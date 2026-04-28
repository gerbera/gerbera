/*GRB*

    Gerbera - https://gerbera.io/

    transcode_int_util.h - this file is part of Gerbera.

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
/// @file transcoding/transcode_int_util.h
#ifndef __TRANSCODE_INT_UTIL_H__
#define __TRANSCODE_INT_UTIL_H__
#ifdef HAVE_FFMPEG

#include <cstddef>
#include <memory>
#include <string>

extern "C" {
struct AVCodecContext;
struct AVFilterContext;
struct AVFilterInOut;
struct AVFilterGraph;
struct AVFormatContext;
struct AVPacket;
struct AVFrame;
struct AVStream;

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

class TranscodingProfile;

class FilteringContext {
private:
    AVCodecContext* decContext = nullptr;
    AVCodecContext* encContext = nullptr;
    std::size_t streamNumber;
    AVFilterInOut* outputs = nullptr;
    AVFilterInOut* inputs = nullptr;
    AVFilterContext* buffersink_ctx = nullptr;
    AVFilterContext* buffersrc_ctx = nullptr;
    AVFilterGraph* filter_graph = nullptr;
    AVPacket* enc_pkt = nullptr;
    AVFrame* filtered_frame = nullptr;

    bool createVideoBuffers();
    bool createAudioBuffers();

public:
    FilteringContext(
        AVCodecContext* dec_ctx,
        AVCodecContext* enc_ctx,
        std::size_t stream_index,
        const std::string& filter_spec,
        const std::shared_ptr<TranscodingProfile>& profile);
    virtual ~FilteringContext();

    operator bool() { return filter_graph; };
    int encodeWriteFrame(
        AVFormatContext* formatContext,
        bool flush);
    int filterEncodeWriteFrame(
        AVFrame* frame,
        AVFormatContext* formatContext);
};

struct TranscodingSettings {
    AVCodecID codecId;
    int width;
    int height;
    AVRational aspectRatio;
    AVPixelFormat pixelFormat;
    AVRational frameRate;
    int sampleRate;
    AVSampleFormat sampleFormat;
    int numChannels;
};

class StreamContext {
private:
    AVStream* stream = nullptr;
    std::size_t streamNumber;
    AVCodecContext* dec_ctx = nullptr;
    AVCodecContext* enc_ctx = nullptr;
    TranscodingSettings settings;

public:
    AVFrame* dec_frame = nullptr;

    StreamContext(
        AVStream* stream,
        std::size_t streamNumber,
        AVFormatContext* inFormatContext,
        const std::shared_ptr<TranscodingProfile>& profile);
    virtual ~StreamContext();

    int createEncoder(AVStream* out_stream, AVFormatContext* outFormatContext);
    int receiveFrame();
    int sendPacket(AVPacket* packet);
    AVCodecContext* getDecoder() { return dec_ctx; }
    AVCodecContext* getEncoder() { return enc_ctx; }

    operator bool() { return dec_ctx && enc_ctx && dec_frame; };
};

#endif // HAVE_FFMPEG
#endif // __TRANSCODE_IO_HANDLER_H__
