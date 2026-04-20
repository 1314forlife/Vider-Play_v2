#ifndef AV_PTR_H
#define AV_PTR_H

#include <memory>
#include "src/stdafx.h"

// FFmpeg 智能指针删除器
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) {
        if (ctx) avformat_close_input(&ctx);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) {
        if (ctx) avcodec_free_context(&ctx);
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) {
        if (frame) av_frame_free(&frame);
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* pkt) {
        if (pkt) av_packet_free(&pkt);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ctx) {
        if (ctx) sws_freeContext(ctx);
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext* ctx) {
        if (ctx) swr_free(&ctx);
    }
};

// 类型别名
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;

// 辅助函数
inline AVPacketPtr make_av_packet() {
    return AVPacketPtr(av_packet_alloc());
}

inline AVFramePtr make_av_frame() {
    return AVFramePtr(av_frame_alloc());
}

#endif