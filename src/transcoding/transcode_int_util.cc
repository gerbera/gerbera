/*GRB*

    Gerbera - https://gerbera.io/

    transcode_int_util.cc - this file is part of Gerbera.

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

/// @file transcoding/transcode_int_util.cc

#ifdef HAVE_FFMPEG
#include "transcode_int_util.h"

#include "config/result/transcoding.h"
#include "util/logger.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#include "util/ffmpeg_compat.h"

#define FILTERNAME_IN "in"
#define FILTERNAME_OUT "out"

FilteringContext::FilteringContext(
    AVCodecContext* dec_ctx,
    AVCodecContext* enc_ctx,
    std::size_t stream_index,
    const std::string& filter_spec,
    const std::shared_ptr<TranscodingProfile>& profile)
    : decContext(dec_ctx)
    , encContext(enc_ctx)
    , streamNumber(stream_index)
{
    int ret = 0;

    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();
    filter_graph = avfilter_graph_alloc();

    if (!outputs || !inputs || !filter_graph) {
        return;
    }

    if (decContext->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (!createVideoBuffers())
            return;
    } else if (decContext->codec_type == AVMEDIA_TYPE_AUDIO) {
        if (!createAudioBuffers())
            return;
    } else {
        return;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup(FILTERNAME_IN);
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup(FILTERNAME_OUT);
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        return;
    }
    ret = avfilter_graph_parse_ptr(filter_graph, filter_spec.c_str(), &inputs, &outputs, nullptr);
    if (ret < 0)
        return;

    ret = avfilter_graph_config(filter_graph, nullptr);
    if (ret < 0)
        return;

    this->enc_pkt = av_packet_alloc();
    if (!this->enc_pkt)
        return;

    this->filtered_frame = av_frame_alloc();
    if (!this->filtered_frame)
        return;
}

FilteringContext::~FilteringContext()
{
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    if (filter_graph) {
        avfilter_graph_free(&filter_graph);
    }
    av_packet_free(&enc_pkt);
    av_frame_free(&filtered_frame);
}

bool FilteringContext::createVideoBuffers()
{
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    if (!buffersrc || !buffersink) {
        log_error("filtering source or sink element not found");
        return false;
    }

    auto args = fmt::format("video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        decContext->width, decContext->height,
        decContext->pix_fmt,
        decContext->pkt_timebase.num, decContext->pkt_timebase.den,
        decContext->sample_aspect_ratio.num, decContext->sample_aspect_ratio.den);

    int ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, FILTERNAME_IN,
        args.c_str(), nullptr, filter_graph);
    if (ret < 0) {
        log_error("Cannot create buffer source");
        return false;
    }

    buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, FILTERNAME_OUT);
    if (!buffersink_ctx) {
        log_error("Cannot create buffer sink");
        return false;
    }

    ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
        (uint8_t*)&encContext->pix_fmt, sizeof(encContext->pix_fmt),
        AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        log_error("Cannot set output pixel format");
    }

    ret = avfilter_init_dict(buffersink_ctx, nullptr);
    if (ret < 0) {
        log_error("Cannot initialize buffer sink");
        return false;
    }
    return true;
}

bool FilteringContext::createAudioBuffers()
{
    const AVFilter* buffersrc = avfilter_get_by_name("abuffer");
    const AVFilter* buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersrc || !buffersink) {
        log_error("filtering source or sink element not found");
        return false;
    }
#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 0, 0))
    if (!decContext->channel_layout)
        decContext->channel_layout = av_get_default_channel_layout(decContext->channels);
    auto buf = decContext->channel_layout;
#else
    char buf[64];
    if (decContext->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
        av_channel_layout_default(&decContext->ch_layout, decContext->ch_layout.nb_channels);

    av_channel_layout_describe(&decContext->ch_layout, buf, sizeof(buf));
#endif

    auto args = fmt::format("time_base={}/{}:sample_rate={}:sample_fmt={}:channel_layout={}",
        decContext->pkt_timebase.num, decContext->pkt_timebase.den, decContext->sample_rate,
        av_get_sample_fmt_name(decContext->sample_fmt),
        buf);

    int ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, FILTERNAME_IN,
        args.c_str(), nullptr, filter_graph);
    if (ret < 0) {
        log_error("Cannot create audio buffer source");
        return false;
    }

    buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, FILTERNAME_OUT);
    if (!buffersink_ctx) {
        log_error("Cannot create audio buffer sink");
        return false;
    }

    ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
        (uint8_t*)&encContext->sample_fmt, sizeof(encContext->sample_fmt),
        AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        log_error("Cannot set output sample format");
        return false;
    }

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 0, 0))
    ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
        (uint8_t*)&encContext->channel_layout,
        sizeof(encContext->channel_layout), AV_OPT_SEARCH_CHILDREN);
#else
    av_channel_layout_describe(&encContext->ch_layout, buf, sizeof(buf));
    ret = av_opt_set(buffersink_ctx, "ch_layouts",
        buf, AV_OPT_SEARCH_CHILDREN);
#endif

    if (ret < 0) {
        log_error("Cannot set output channel layout");
        return false;
    }

    ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
        (uint8_t*)&encContext->sample_rate, sizeof(encContext->sample_rate),
        AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        log_error("Cannot set output sample rate");
        return false;
    }

    if (encContext->frame_size > 0)
        av_buffersink_set_frame_size(buffersink_ctx, encContext->frame_size);

    ret = avfilter_init_dict(buffersink_ctx, nullptr);
    if (ret < 0) {
        log_error("Cannot initialize audio buffer sink");
        return false;
    }
    return true;
}

int FilteringContext::encodeWriteFrame(
    AVFormatContext* formatContext,
    bool flush)
{
    AVFrame* filteredFrame = flush ? nullptr : this->filtered_frame;
    AVPacket* encPacket = this->enc_pkt;
    int ret;

    log_debug("Encoding frame");
    /* encode filtered frame */
    av_packet_unref(encPacket);

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0))
    if (filteredFrame && filteredFrame->pts != AV_NOPTS_VALUE)
        filteredFrame->pts = av_rescale_q(filteredFrame->pts, filteredFrame->time_base,
            this->encContext->time_base);
#endif

    ret = avcodec_send_frame(this->encContext, filteredFrame);

    if (ret < 0)
        return ret;

    while (ret >= 0) {
        ret = avcodec_receive_packet(this->encContext, encPacket);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;

        /* prepare packet for muxing */
        encPacket->stream_index = this->streamNumber;
        av_packet_rescale_ts(encPacket, this->encContext->time_base, formatContext->streams[this->streamNumber]->time_base);

        log_debug("Muxing frame");
        /* mux encoded frame */
        ret = av_interleaved_write_frame(formatContext, encPacket);
    }

    return ret;
}

int FilteringContext::filterEncodeWriteFrame(
    AVFrame* frame,
    AVFormatContext* formatContext)
{
    int ret;

    log_debug("Pushing decoded frame to filters");
    /* push the decoded frame into the filtergraph */
    ret = av_buffersrc_add_frame_flags(this->buffersrc_ctx, frame, 0);
    if (ret < 0) {
        log_error("Error while feeding the filtergraph");
        return ret;
    }

    /* pull filtered frames from the filtergraph */
    while (1) {
        log_debug("Pulling filtered frame from filters");
        ret = av_buffersink_get_frame(this->buffersink_ctx, this->filtered_frame);
        if (ret < 0) {
            /* if no more frames for output - returns AVERROR(EAGAIN)
             * if flushed and no more frames for output - returns AVERROR_EOF
             * rewrite retcode to 0 to show it as normal procedure completion
             */
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            break;
        }

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0))
        this->filtered_frame->time_base = av_buffersink_get_time_base(this->buffersink_ctx);
#endif

        this->filtered_frame->pict_type = AV_PICTURE_TYPE_NONE;
        ret = this->encodeWriteFrame(formatContext, false);
        av_frame_unref(this->filtered_frame);
        if (ret < 0)
            break;
    }

    return ret;
}

StreamContext::StreamContext(
    AVStream* stream,
    std::size_t streamNumber,
    AVFormatContext* inFormatContext,
    const std::shared_ptr<TranscodingProfile>& profile)
    : stream(stream)
    , streamNumber(streamNumber)
{
    const AVCodec* dec = avcodec_find_decoder(as_codecpar(stream)->codec_id);
    if (!dec) {
        log_error("Failed to find decoder for stream #{}", streamNumber);
        return;
    }
    this->dec_ctx = avcodec_alloc_context3(dec);
    if (!this->dec_ctx) {
        log_error("Failed to allocate the decoder context for stream #{}", streamNumber);
        return;
    }
    int ret = avcodec_parameters_to_context(this->dec_ctx, as_codecpar(stream));
    if (ret < 0) {
        log_error("Failed to copy decoder parameters to input decoder context for stream #{}", streamNumber);
        return;
    }

    /* Inform the decoder about the timebase for the packet timestamps.
     * This is highly recommended, but not mandatory. */
    this->dec_ctx->pkt_timebase = stream->time_base;

    /* Reencode video & audio and remux subtitles etc. */
    if (this->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
        || this->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        if (this->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
            this->dec_ctx->framerate = av_guess_frame_rate(inFormatContext, stream, nullptr);
        /* Open decoder */
        ret = avcodec_open2(this->dec_ctx, dec, nullptr);
        if (ret < 0) {
            log_error("Failed to open decoder for stream #{}", streamNumber);
            return;
        }
    }
    this->dec_frame = av_frame_alloc();

    if (!this->dec_frame)
        return;

    /* as default, we choose transcoding to same codec */
    settings.codecId = dec_ctx->codec_id;
    /* as default, we transcode to same properties (picture size,
     * sample rate etc.). These properties can be changed for output
     * streams easily using filters */
    settings.height = dec_ctx->height;
    settings.width = dec_ctx->width;
    settings.aspectRatio = dec_ctx->sample_aspect_ratio;
    settings.pixelFormat = dec_ctx->pix_fmt;
    settings.frameRate = dec_ctx->framerate;
    settings.sampleRate = dec_ctx->sample_rate;
    settings.sampleFormat = dec_ctx->sample_fmt;

    if (profile->getSampleFreq() != SOURCE)
        settings.sampleRate = profile->getSampleFreq(); // fix with bits_per_sample
    if (profile->getNumChannels() != SOURCE)
        settings.numChannels = profile->getNumChannels();
    if (profile->encoder.getWidth() != SOURCE)
        settings.width = profile->encoder.getWidth();
    if (profile->encoder.getHeight() != SOURCE)
        settings.height = profile->encoder.getHeight();

    if (this->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO && !profile->encoder.getACodec().empty()) {
        auto codec = avcodec_find_encoder_by_name(profile->encoder.getACodec().c_str());
        if (codec)
            settings.codecId = codec->id;
    } else if (this->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO && !profile->encoder.getVCodec().empty()) {
        auto codec = avcodec_find_encoder_by_name(profile->encoder.getVCodec().c_str());
        if (codec)
            settings.codecId = codec->id;
    }
}

StreamContext::~StreamContext()
{
    if (dec_ctx)
        avcodec_free_context(&dec_ctx);
    if (enc_ctx)
        avcodec_free_context(&enc_ctx);
    if (dec_frame)
        av_frame_free(&dec_frame);
}

int StreamContext::receiveFrame()
{
    return avcodec_receive_frame(this->dec_ctx, this->dec_frame);
}
int StreamContext::sendPacket(AVPacket* packet)
{
    return avcodec_send_packet(this->dec_ctx, packet);
}

int StreamContext::createEncoder(
    AVStream* out_stream,
    AVFormatContext* outFormatContext)
{
    int ret = 0;

    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
        || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        const AVCodec* encoder = avcodec_find_encoder(settings.codecId);
        if (!encoder) {
            log_error("Necessary encoder {} not found", settings.codecId);
            return AVERROR_INVALIDDATA;
        }
        enc_ctx = avcodec_alloc_context3(encoder);
        if (!enc_ctx) {
            log_error("Failed to allocate the encoder context");
            return AVERROR(ENOMEM);
        }

        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            enc_ctx->height = settings.height;
            enc_ctx->width = settings.height;
            enc_ctx->sample_aspect_ratio = settings.aspectRatio;

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 0, 0))
            /* take first format from list of supported formats */
            if (encoder->pix_fmts)
                enc_ctx->pix_fmt = encoder->pix_fmts[0];
            else
                enc_ctx->pix_fmt = dec_ctx->pix_fmt;
#else
            const enum AVPixelFormat* pix_fmts = nullptr;
            ret = avcodec_get_supported_config(enc_ctx, nullptr,
                AV_CODEC_CONFIG_PIX_FORMAT, 0,
                (const void**)&pix_fmts, nullptr);

            /* take first format from list of supported formats */
            enc_ctx->pix_fmt = (ret >= 0 && pix_fmts) ? pix_fmts[0] : settings.pixelFormat;
#endif

            /* video time_base can be set to whatever is handy and supported by encoder */
            enc_ctx->time_base = av_inv_q(settings.frameRate);
        } else {
            enc_ctx->sample_rate = settings.sampleRate;

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 0, 0))
            enc_ctx->channel_layout = dec_ctx->channel_layout;
            enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
#else
            ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
            if (ret < 0)
                return ret;
#endif

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 0, 0))
            if (encoder->sample_fmts)
                enc_ctx->sample_fmt = encoder->sample_fmts[0];
#else
            const enum AVSampleFormat* sample_fmts = nullptr;
            ret = avcodec_get_supported_config(enc_ctx, nullptr,
                AV_CODEC_CONFIG_SAMPLE_FORMAT, 0,
                (const void**)&sample_fmts, nullptr);

            /* take first format from list of supported formats */
            enc_ctx->sample_fmt = (ret >= 0 && sample_fmts) ? sample_fmts[0] : settings.sampleFormat;
#endif

            enc_ctx->time_base = (AVRational) { 1, enc_ctx->sample_rate };
        }

        if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        /* Third parameter can be used to pass settings to encoder */
        ret = avcodec_open2(enc_ctx, encoder, nullptr);
        if (ret < 0) {
            log_error("Cannot open {} encoder for stream #{}", encoder->name, streamNumber);
            return ret;
        }
        ret = avcodec_parameters_from_context(as_codecpar(out_stream), enc_ctx);
        if (ret < 0) {
            log_error("Failed to copy encoder parameters to output stream #{}", streamNumber);
            return ret;
        }

        out_stream->time_base = enc_ctx->time_base;
    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
        log_error("Elementary stream #{} is of unknown type, cannot proceed", streamNumber);
        return AVERROR_INVALIDDATA;
    } else {
        /* if this stream must be remuxed */
        ret = avcodec_parameters_copy(as_codecpar(out_stream), as_codecpar(stream));
        if (ret < 0) {
            log_error("Copying parameters for stream #{} failed\n", streamNumber);
            return ret;
        }
        out_stream->time_base = stream->time_base;
    }

    return ret;
}

#endif
