#ifndef PLAY_ENGINE_H
#define PLAY_ENGINE_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include <QMap>           // 🔥 添加
#include <functional>     // 🔥 添加
#include "playback_state.h"
#include "src/core/frame.h"
#include "src/common/thread_safe_queue.h"
#include "src/engine/video_decode_thread.h"
#include "src/engine/audio_decode_thread.h"
#include "src/engine/render_prepare_thread.h"
#include "src/engine/demux_thread.h"
#include "src/plugins/audio_renderer/sdl_audio_renderer.h"

class VideoDecoder;
class SDLRenderer;
class QThread;

struct StreamInfo {
    QString url;          // 子播放列表地址
    int bandwidth;        // 码率 (bps)
    int width;
    int height;
    QString resolution;   // "1080p"
    int index;
};

class PlayEngine : public QObject {
    Q_OBJECT
public:
    static PlayEngine& instance();

    bool openFile(const QString& filePath);
    bool openStream(const QString& url);
    void setRenderWindow(void* winId);
    void setRenderWindowSize(int width, int height);

    void play();
    void pause();
    void stop();
    void seek(int64_t position);

    void setVolume(int volume);  // 0-100
    int volume() const;

    PlaybackState state() const { return m_state; }
    bool isPlaying() const { return m_state == PlaybackState::Playing; }
    bool isPaused() const { return m_state == PlaybackState::Paused; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    double fps() const { return m_fps; }
    int64_t duration() const { return m_duration; }
    // 注册模块
    void registerDecoder(const QString& name, std::function<std::unique_ptr<VideoDecoder>()> factory);
    void registerRenderer(const QString& name, std::function<std::unique_ptr<SDLRenderer>()> factory);

    // 切换模块
    bool setDecoder(const QString& name);
    bool setRenderer(const QString& name);

    bool parseMasterPlaylist(const QString& content);
    // 新增：设置多码率流列表（打开 master.m3u8 时调用）
    void setStreams(const QVector<StreamInfo>& streams);
    // 新增：获取所有流信息
    const QVector<StreamInfo>& getStreams() const { return m_streams; }
    // 新增：切换到指定清晰度
    void switchToStream(int streamIndex);
    // 新增：获取当前清晰度索引
    int currentStreamIndex() const { return m_currentStreamIndex; }

signals:
    void sigStateChanged(PlaybackState state);
    void sigProgressChanged(int64_t current, int64_t total);  // 🔥 如果需要进度条
    void sigError(const QString& error);
    void sigFrameRendered();  // 🔥 如果需要帧统计
    // 新增：清晰度切换完成
    void streamSwitched(int newIndex);
    void qualityStreamsReady(const QVector<StreamInfo>& streams, int defaultIndex);

private slots:
    void onRenderTimer();
    void onFrameDecoded();           // 视频帧解码
    void onAudioFrameDecoded();      // 音频帧解码
    void onDecodeFinished();         // 通用完成（可选）
    void onStreamsReady();
private:
    PlayEngine(QObject* parent = nullptr);
    ~PlayEngine();

    void initDecodeThread();
    void stopDecodeThread();

    // 渲染命令双缓冲
    RenderCommand m_renderCmds[2];
    std::atomic<RenderCommand*> m_currRenderCmd{nullptr};
    std::atomic<int> m_writeCmdIdx{0};

    // 🔥 视频模块
    std::unique_ptr<VideoDecoder> m_videoDecoder;
    std::unique_ptr<VideoDecodeThread> m_videoDecodeThread;
    std::unique_ptr<ThreadSafeQueue<FrameData>> m_videoFrameQueue;
    std::unique_ptr<SDLAudioRenderer> m_audioRenderer;

    // 🔥 音频模块
    std::unique_ptr<AudioDecoder> m_audioDecoder;
    std::unique_ptr<AudioDecodeThread> m_audioDecodeThread;
    std::unique_ptr<ThreadSafeQueue<AudioFrame>> m_audioFrameQueue;

    // 渲染器
    std::unique_ptr<SDLRenderer> m_renderer;
    void* m_renderWindowId = nullptr;
    QTimer* m_renderTimer = nullptr;

    // 渲染准备线程
    std::unique_ptr<RenderPrepareThread> m_renderPrepareThread;
    //std::unique_ptr<ThreadSafeQueue<FrameData>> m_renderCommandQueue;

    // 音频时钟（用于同步）
    std::atomic<int64_t> m_audioClock{0};

    std::unique_ptr<DemuxThread> m_demuxThread;
    std::unique_ptr<ThreadSafeQueue<Packet>> m_videoPacketQueue;  // 视频包队列
    std::unique_ptr<ThreadSafeQueue<Packet>> m_audioPacketQueue;  // 音频包队列

    PlaybackState m_state = PlaybackState::Stopped;

    int m_width = 0;
    int m_height = 0;
    double m_fps = 25.0;
    int64_t m_duration = 0;

    // 性能统计
    QElapsedTimer m_statsTimer;
    int m_videoFrameCount = 0;
    int m_videoDecodeCount = 0;
    int m_audioDecodeCount = 0;
    int64_t m_currentPts = 0;

    int m_volume = 100;

    // 注册表
    QMap<QString, std::function<std::unique_ptr<VideoDecoder>()>> m_videoDecoderFactories;
    QMap<QString, std::function<std::unique_ptr<SDLRenderer>()>> m_rendererFactories;

    static PlayEngine* s_instance;

    QVector<StreamInfo> m_streams;
    int m_currentStreamIndex = -1;
    bool m_isMasterPlaylist = false;
    QString m_currentStreamUrl;

    QString m_masterUrl;
};

#endif