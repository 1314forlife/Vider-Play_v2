#ifndef FRAME_H
#define FRAME_H

#include <memory>
#include <cstdint>
#include <cstring>
#include <atomic>  // 🔥 添加

extern "C" {
#include <libavutil/frame.h>
}

class FrameData {
public:
    FrameData() = default;

    // 移动构造
    FrameData(FrameData&& other) noexcept = default;
    FrameData& operator=(FrameData&& other) noexcept = default;

    // 禁止拷贝
    FrameData(const FrameData&) = delete;
    FrameData& operator=(const FrameData&) = delete;

    // 从 AVFrame 移动构造（零拷贝）
    static FrameData fromAVFrame(AVFrame* frame, int64_t pts) {
        FrameData data;
        data.m_frame = std::shared_ptr<AVFrame>(frame, [](AVFrame* f) {
            if (f) av_frame_free(&f);
        });
        data.m_pts = pts;
        data.m_width = frame->width;
        data.m_height = frame->height;

        for (int i = 0; i < 4; i++) {
            data.m_linesize[i] = frame->linesize[i];
        }

        return data;
    }

    // 获取 YUV 数据指针
    uint8_t* getY() const {
        return m_frame ? m_frame->data[0] : nullptr;
    }

    uint8_t* getU() const {
        return m_frame ? m_frame->data[1] : nullptr;
    }

    uint8_t* getV() const {
        return m_frame ? m_frame->data[2] : nullptr;
    }

    int* linesize() { return m_linesize; }
    const int* linesize() const { return m_linesize; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int64_t pts() const { return m_pts; }

    bool isValid() const { return m_frame != nullptr && m_width > 0 && m_height > 0; }

private:
    std::shared_ptr<AVFrame> m_frame;
    int m_linesize[4] = {0};
    int m_width = 0;
    int m_height = 0;
    int64_t m_pts = 0;
};

// 🔥 新增：渲染命令结构（双缓冲用）
struct RenderCommand {
    uint8_t* y = nullptr;
    uint8_t* u = nullptr;
    uint8_t* v = nullptr;
    int yLinesize = 0;
    int uLinesize = 0;
    int vLinesize = 0;
    int64_t pts = 0;
    int width = 0;
    int height = 0;
    bool valid = false;

    void set(const FrameData& frame) {
        y = frame.getY();
        u = frame.getU();
        v = frame.getV();
        yLinesize = frame.linesize()[0];
        uLinesize = frame.linesize()[1];
        vLinesize = frame.linesize()[2];
        pts = frame.pts();
        width = frame.width();
        height = frame.height();
        valid = true;
    }

    void clear() {
        valid = false;
    }
};

#endif