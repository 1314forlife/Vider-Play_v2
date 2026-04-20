#ifndef DEMUX_THREAD_H
#define DEMUX_THREAD_H

#include <QThread>
#include <atomic>
#include <memory>
#include "src/common/thread_safe_queue.h"
#include "src/core/packet.h"

extern "C" {
#include <libavformat/avformat.h>
}

class DemuxThread : public QThread {
    Q_OBJECT
public:
    explicit DemuxThread(QObject* parent = nullptr);
    ~DemuxThread();

    bool isNetworkStream() const { return m_isNetworkStream; }

    bool open(const QString& filePath);
    void close();
    bool seek(int64_t timestampUs);

    void setVideoPacketQueue(ThreadSafeQueue<Packet>* queue) { m_videoPacketQueue = queue; }
    void setAudioPacketQueue(ThreadSafeQueue<Packet>* queue) { m_audioPacketQueue = queue; }

    void startThread();
    void stopThread();
    void pauseThread();
    void resumeThread();

    // 流信息
    int videoStreamIndex() const { return m_videoStreamIndex; }
    int audioStreamIndex() const { return m_audioStreamIndex; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    double fps() const { return m_fps; }
    int64_t duration() const { return m_duration; }
    int sampleRate() const { return m_sampleRate; }
    int channels() const { return m_channels; }

    // 获取流参数
    AVCodecParameters* getVideoCodecPar() const { return m_videoCodecPar; }
    AVCodecParameters* getAudioCodecPar() const { return m_audioCodecPar; }
    AVRational getVideoTimeBase() const { return m_videoTimeBase; }
    AVRational getAudioTimeBase() const { return m_audioTimeBase; }

signals:
    void sigStreamsReady();  // 流信息准备就绪
    void sigError(const QString& error);

protected:
    void run() override;

private:
    AVFormatContext* m_formatCtx = nullptr;

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;


    // 流参数（需要保存，因为 formatCtx 可能被关闭）
    AVCodecParameters* m_videoCodecPar = nullptr;
    AVCodecParameters* m_audioCodecPar = nullptr;
    AVRational m_videoTimeBase{0, 1};
    AVRational m_audioTimeBase{0, 1};

    int m_width = 0;
    int m_height = 0;
    double m_fps = 25.0;
    int64_t m_duration = 0;
    int m_sampleRate = 0;
    int m_channels = 0;

    ThreadSafeQueue<Packet>* m_videoPacketQueue = nullptr;
    ThreadSafeQueue<Packet>* m_audioPacketQueue = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_eof{false};

    bool m_isNetworkStream = false;
};

#endif