#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "audio_decoder_base.h"
#include "src/core/audio_frame.h"
#include "src/core/packet.h"
#include "src/common/thread_safe_queue.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class AudioDecoder : public AudioDecoderBase {
    Q_OBJECT
public:
    explicit AudioDecoder(QObject* parent = nullptr);
    ~AudioDecoder();
    void flush();
    // 🔥 实现基类所有纯虚函数
    bool open(const QString& url) override { return false; }  // 不使用，但必须实现
    void close() override;
    bool readFrame(AudioFrame& frame) override { return decodeFrame(frame); }  // 🔥 调用 decodeFrame
    bool isEOF() const override { return m_eof; }

    int sampleRate() const override { return m_sampleRate; }
    int channels() const override { return m_channels; }
    int64_t duration() const override { return m_duration; }

    // 新接口
    bool open(AVCodecParameters* codecPar, AVRational timeBase);
    bool decodeFrame(AudioFrame& frame);
    void setPacketQueue(ThreadSafeQueue<Packet>* queue) { m_packetQueue = queue; }
    bool isOpen() const { return m_isOpen; }

    int getPacketQueueSize() const { return m_packetQueue ? m_packetQueue->size() : 0; }
private:
    bool initDecoder(AVCodecParameters* codecPar);
    bool decodePacket(AVPacket* packet, AudioFrame& frame);
    int64_t rescalePts(int64_t pts, AVRational from, AVRational to);

    AVCodecContext* m_codecCtx = nullptr;
    SwrContext* m_swrCtx = nullptr;
    uint8_t** m_resampledData = nullptr;   // 🔥 重采样后的数据缓冲区
    int m_resampledLinesize = 0;           // 🔥 缓冲区大小
    AVFrame* m_frame = nullptr;

    int m_sampleRate = 0;
    int m_channels = 0;
    int64_t m_duration = 0;
    AVRational m_timeBase{1, 1000000};

    ThreadSafeQueue<Packet>* m_packetQueue = nullptr;

    bool m_isOpen = false;
    bool m_eof = false;
};

#endif