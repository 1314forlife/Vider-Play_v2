#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include "decoder_base.h"
#include "src/core/packet.h"
#include "src/core/frame.h"
#include "src/common/thread_safe_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

/**
 * 视频解码器插件
 * 职责：
 * 1. 从包队列读取编码包
 * 2. 解码视频流
 * 3. 输出 FrameData 到环形缓冲
 */
class VideoDecoder : public DecoderBase {
    Q_OBJECT
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

    // 🔥 修改：不再直接打开文件，而是初始化解码器
    bool open(AVCodecParameters* codecPar, AVRational timeBase);
    void close() override;

    // 🔥 修改：从包队列解码一帧
    bool decodeFrame(FrameData& frame);

    // 保留兼容性（但不再使用）
    bool open(const QString& url) override { return false; }
    bool readFrame(FrameData& frame) override { return false; }
    bool seek(int64_t timestampUs);

    void flush();

    // 视频信息
    int width() const { return m_width; }
    int height() const { return m_height; }
    double fps() const { return m_fps; }
    int64_t duration() const { return m_duration; }

    // 🔥 新增：设置包队列
    void setPacketQueue(ThreadSafeQueue<Packet>* queue) { m_packetQueue = queue; }

    // 是否到达文件末尾
    bool isEOF() const { return m_eof; }

private:
    bool initDecoder(AVCodecParameters* codecPar);
    bool decodePacket(AVPacket* packet, FrameData& frame);
    int64_t rescalePts(int64_t pts, AVRational from, AVRational to);

    // FFmpeg 上下文
    AVCodecContext* m_codecCtx = nullptr;
    AVFrame* m_frame = nullptr;

    // 视频参数
    int m_width = 0;
    int m_height = 0;
    double m_fps = 25.0;
    int64_t m_duration = 0;
    AVRational m_timeBase{1, 1000000};

    // 🔥 新增：包队列
    ThreadSafeQueue<Packet>* m_packetQueue = nullptr;

    // 状态
    bool m_isOpen = false;
    bool m_eof = false;
};

#endif