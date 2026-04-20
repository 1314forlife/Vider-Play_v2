#include "play_engine.h"
#include "src/plugins/audio_decoder/audio_decoder.h"
#include "src/plugins/video_decoder/video_decoder.h"
#include "src/plugins/video_renderer/sdl_renderer.h"
#include "src/engine/video_decode_thread.h"
#include "src/engine/audio_decode_thread.h"
#include "src/engine/thread_registry.h"
#include "src/common/logger.h"

PlayEngine* PlayEngine::s_instance = nullptr;

PlayEngine::PlayEngine(QObject* parent) : QObject(parent) {
    ThreadRegistry::instance().initDefaultThreads();

    // 视频帧队列（解码 → 渲染准备）
    m_videoFrameQueue = std::make_unique<ThreadSafeQueue<FrameData>>(10);

    // 音频帧队列
    m_audioFrameQueue = std::make_unique<ThreadSafeQueue<AudioFrame>>(10);

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &PlayEngine::onRenderTimer);

    // 初始化双缓冲
    m_currRenderCmd.store(nullptr);
    m_writeCmdIdx.store(0);

    // 创建渲染准备线程
    m_renderPrepareThread = std::make_unique<RenderPrepareThread>();
    m_renderPrepareThread->setInputQueue(m_videoFrameQueue.get());
    m_renderPrepareThread->setRenderCmds(m_renderCmds, &m_currRenderCmd, &m_writeCmdIdx);
    m_renderPrepareThread->setAudioClock(&m_audioClock);

    registerDecoder("ffmpeg", []() { return std::make_unique<VideoDecoder>(); });
    registerRenderer("sdl", []() { return std::make_unique<SDLRenderer>(); });
}

PlayEngine::~PlayEngine() {
    stop();
}

PlayEngine& PlayEngine::instance() {
    if (!s_instance) {
        s_instance = new PlayEngine();
    }
    return *s_instance;
}

void PlayEngine::registerDecoder(const QString& name,
                                 std::function<std::unique_ptr<VideoDecoder>()> factory) {
    m_videoDecoderFactories[name] = factory;
    LOG_INFO("PlayEngine", "注册解码器: " + name);
}

void PlayEngine::registerRenderer(const QString& name,
                                  std::function<std::unique_ptr<SDLRenderer>()> factory) {
    m_rendererFactories[name] = factory;
    LOG_INFO("PlayEngine", "注册渲染器: " + name);
}

bool PlayEngine::setDecoder(const QString& name) {
    if (!m_videoDecoderFactories.contains(name)) {
        LOG_ERROR("PlayEngine", "解码器不存在: " + name);
        return false;
    }
    m_videoDecoder = m_videoDecoderFactories[name]();
    LOG_INFO("PlayEngine", "切换解码器: " + name);
    return true;
}

bool PlayEngine::setRenderer(const QString& name) {
    if (!m_rendererFactories.contains(name)) {
        LOG_ERROR("PlayEngine", "渲染器不存在: " + name);
        return false;
    }
    m_renderer = m_rendererFactories[name]();
    if (m_renderWindowId) {
        m_renderer->init(m_renderWindowId, m_width, m_height);
    }
    LOG_INFO("PlayEngine", "切换渲染器: " + name);
    return true;
}

bool PlayEngine::openFile(const QString& filePath) {
    stop();

    // 1. 创建包队列（解复用 → 解码器）
    m_videoPacketQueue = std::make_unique<ThreadSafeQueue<Packet>>(100);
    m_audioPacketQueue = std::make_unique<ThreadSafeQueue<Packet>>(100);

    LOG_INFO("PlayEngine", QString("创建视频包队列，地址: %1").arg((quintptr)m_videoPacketQueue.get()));
    LOG_INFO("PlayEngine", QString("创建音频包队列，地址: %1").arg((quintptr)m_audioPacketQueue.get()));
    m_demuxThread.reset(static_cast<DemuxThread*>(
        ThreadRegistry::instance().createThread(ThreadType::Demux)));

    if (!m_demuxThread) {
        LOG_ERROR("PlayEngine", "无法创建解复用线程");
        return false;
    }

    m_demuxThread->setVideoPacketQueue(m_videoPacketQueue.get());
    m_demuxThread->setAudioPacketQueue(m_audioPacketQueue.get());

    connect(m_demuxThread.get(), &DemuxThread::sigStreamsReady,
            this, &PlayEngine::onStreamsReady);
    connect(m_demuxThread.get(), &DemuxThread::sigError,
            this, &PlayEngine::sigError);

    // 3. 打开文件（解复用线程会解析流信息）
    if (!m_demuxThread->open(filePath)) {
        LOG_ERROR("PlayEngine", "打开文件失败: " + filePath);
        return false;
    }

    return true;
}

void PlayEngine::setRenderWindow(void* winId) {
    m_renderWindowId = winId;

    if (m_videoDecoder && !m_renderer) {
        m_renderer = std::make_unique<SDLRenderer>();
        m_renderer->init(m_renderWindowId, m_width, m_height);
    }
}

void PlayEngine::setRenderWindowSize(int width, int height) {
    if (m_renderer) {
        m_renderer->resize(width, height);
    }
}

void PlayEngine::play() {
    if (!m_videoDecoder || !m_renderer) {
        emit sigError("未打开文件或未设置渲染窗口");
        return;
    }

    if (!m_audioDecoder) {
        emit sigError("未打开文件");
        return;
    }

    if (m_state == PlaybackState::Paused) {
        m_state = PlaybackState::Playing;
        m_renderTimer->setTimerType(Qt::PreciseTimer);
        m_renderTimer->start(1000 / m_fps);

        if (m_videoDecodeThread) {
            m_videoDecodeThread->resumeThread();
        }

        if (m_renderPrepareThread) {
            m_renderPrepareThread->resumeThread();
        }

        if (m_audioRenderer) {
            m_audioRenderer->play();
        }

        emit sigStateChanged(m_state);
        LOG_INFO("PlayEngine", "恢复播放");
    } else if (m_state == PlaybackState::Stopped) {
        m_state = PlaybackState::Playing;

        m_videoFrameQueue->clear();

        if (m_audioFrameQueue) {
            m_audioFrameQueue->clear();
        }

        m_currentPts = 0;
        m_videoFrameCount = 0;
        m_videoDecodeCount = 0;
        m_audioDecodeCount = 0;
        m_statsTimer.restart();

        if (m_videoDecodeThread) {
            m_videoDecodeThread->startThread();
        }

        if (m_audioDecodeThread) {
            m_audioDecodeThread->startThread();
        }
        if (m_demuxThread) {
            m_demuxThread->startThread();
        }

        if (m_renderPrepareThread) {
            m_renderPrepareThread->startThread();
        }

        m_renderTimer->setTimerType(Qt::PreciseTimer);
        m_renderTimer->start(1000 / m_fps);

        if (m_audioRenderer) {
            m_audioRenderer->play();
        }

        emit sigStateChanged(m_state);
        LOG_INFO("PlayEngine", "开始播放");
    }
}

void PlayEngine::pause() {
    if (m_state != PlaybackState::Playing) return;

    m_state = PlaybackState::Paused;
    m_renderTimer->stop();

    if (m_videoDecodeThread) {
        m_videoDecodeThread->pauseThread();
    }

    if (m_audioDecodeThread) {
        m_audioDecodeThread->pauseThread();
    }
    if (m_audioRenderer) {
        m_audioRenderer->pause();
    }
    if (m_demuxThread) {
        m_demuxThread->pauseThread();
    }
    if (m_renderPrepareThread) {
        m_renderPrepareThread->pauseThread();
    }

    emit sigStateChanged(m_state);
    LOG_INFO("PlayEngine", "暂停");
}

void PlayEngine::stop() {
    if (m_state == PlaybackState::Stopped) return;

    m_state = PlaybackState::Stopped;
    m_renderTimer->stop();

    if (m_videoDecodeThread) {
        m_videoDecodeThread->stopThread();
    }

    if (m_audioDecodeThread) {
        m_audioDecodeThread->stopThread();
    }
    if (m_renderPrepareThread) {
        m_renderPrepareThread->stopThread();
    }
    if (m_audioRenderer) {
        m_audioRenderer->stop();
    }

    if (m_videoFrameQueue) {
        m_videoFrameQueue->clear();
    }

    if (m_audioFrameQueue) {
        m_audioFrameQueue->clear();
    }

    if (m_videoDecoder) {
        m_videoDecoder->close();
    }

    if (m_audioDecoder) {
        m_audioDecoder->close();
    }

    if (m_renderer) {
        m_renderer->shutdown();
    }

    if (m_demuxThread) {
        m_demuxThread->stopThread();
    }

    m_currentPts = 0;

    emit sigStateChanged(m_state);
    LOG_INFO("PlayEngine", "停止");
}

void PlayEngine::seek(int64_t position) {
    LOG_INFO("PlayEngine", QString("Seek 到: %1 ms").arg(position / 1000));

    if (!m_demuxThread) {
        LOG_WARN("PlayEngine", "解复用线程未初始化");
        return;
    }

    // 1. 暂停所有线程
    if (m_demuxThread) {
        m_demuxThread->pauseThread();
    }
    if (m_videoDecodeThread) {
        m_videoDecodeThread->pauseThread();
    }
    if (m_audioDecodeThread) {
        m_audioDecodeThread->pauseThread();
    }

    // 2. 清空所有队列
    if (m_videoPacketQueue) {
        m_videoPacketQueue->clear();
    }
    if (m_audioPacketQueue) {
        m_audioPacketQueue->clear();
    }
    if (m_videoFrameQueue) {
        m_videoFrameQueue->clear();
    }
    if (m_audioFrameQueue) {
        m_audioFrameQueue->clear();
    }

    // 3. 执行 seek
    if (m_demuxThread->seek(position)) {
        m_currentPts = position;
        LOG_INFO("PlayEngine", "Seek 成功");
    } else {
        LOG_WARN("PlayEngine", "Seek 失败");
    }

    // 4. 恢复所有线程
    if (m_state == PlaybackState::Playing) {
        if (m_demuxThread) {
            m_demuxThread->resumeThread();
        }
        if (m_videoDecodeThread) {
            m_videoDecodeThread->resumeThread();
        }
        if (m_audioDecodeThread) {
            m_audioDecodeThread->resumeThread();
        }
    }
}

void PlayEngine::onRenderTimer() {
    if (m_state != PlaybackState::Playing) return;
    if (!m_renderer) return;

    // 从双缓冲读取渲染命令
    RenderCommand* cmd = m_currRenderCmd.exchange(nullptr);
    if (cmd && cmd->valid) {
        m_renderer->renderYUV(
            cmd->y, cmd->yLinesize,
            cmd->u, cmd->uLinesize,
            cmd->v, cmd->vLinesize,
            cmd->pts
            );
        m_currentPts = cmd->pts;
        m_videoFrameCount++;
        emit sigProgressChanged(m_currentPts, m_duration);
        emit sigFrameRendered();
    }

    // 统计（每秒一次）
    if (m_statsTimer.elapsed() >= 1000) {
        LOG_INFO("PlayEngine",
                 QString("渲染 FPS: %1, 视频队列: %2, 目标 FPS: %3")
                     .arg(m_videoFrameCount)
                     .arg(m_videoFrameQueue->size())
                     .arg(m_fps));
        m_videoFrameCount = 0;
        m_statsTimer.restart();
    }
}

void PlayEngine::onFrameDecoded() {
    m_videoDecodeCount++;
}

void PlayEngine::onAudioFrameDecoded() {
    m_audioDecodeCount++;
}

void PlayEngine::onDecodeFinished() {
    LOG_INFO("PlayEngine", "解码线程完成");
}

void PlayEngine::onStreamsReady()
{
    if (!m_demuxThread) return;

    // 获取视频流参数
    AVCodecParameters* videoCodecPar = m_demuxThread->getVideoCodecPar();
    AVRational videoTimeBase = m_demuxThread->getVideoTimeBase();

    // 初始化视频解码器
    m_videoDecoder = std::make_unique<VideoDecoder>();
    if (!m_videoDecoder->open(videoCodecPar, videoTimeBase)) {
        emit sigError("视频解码器初始化失败");
        return;
    }
    m_videoDecoder->setPacketQueue(m_videoPacketQueue.get());

    // 获取视频信息
    m_width = m_demuxThread->width();
    m_height = m_demuxThread->height();
    m_fps = m_demuxThread->fps();
    m_duration = m_demuxThread->duration();

    if (m_width == 0 || m_height == 0) {
        LOG_WARN("PlayEngine", QString("获取到无效尺寸 %1x%2，使用默认值 1280x720")
                                   .arg(m_width).arg(m_height));
        m_width = 1280;
        m_height = 720;
    }
    if (m_fps == 0) {
        m_fps = 25;
    }

    // 获取音频流参数
    AVCodecParameters* audioCodecPar = m_demuxThread->getAudioCodecPar();

    LOG_INFO("PlayEngine", QString("音频流参数存在: %1").arg(audioCodecPar ? "是" : "否"));

    LOG_INFO("PlayEngine", QString("onStreamsReady: 音频包队列地址: %1").arg((quintptr)m_audioPacketQueue.get()));
    if (audioCodecPar) {
        AVRational audioTimeBase = m_demuxThread->getAudioTimeBase();

        // 初始化音频解码器
        m_audioDecoder = std::make_unique<AudioDecoder>();
        LOG_INFO("PlayEngine", "开始打开音频解码器...");
        if (m_audioDecoder->open(audioCodecPar, audioTimeBase)) {
            LOG_INFO("PlayEngine", "准备调用 setPacketQueue");
            m_audioDecoder->setPacketQueue(m_audioPacketQueue.get());

            // 初始化音频渲染器
            if (!m_audioRenderer) {
                m_audioRenderer = std::make_unique<SDLAudioRenderer>();
            }
            m_audioRenderer->init(m_audioDecoder->sampleRate(), m_audioDecoder->channels());
            m_audioRenderer->setAudioQueue(m_audioFrameQueue.get());
        }else{
            LOG_ERROR("PlayEngine", "音频解码器打开失败！");
        }
    }else {
        LOG_WARN("PlayEngine", "没有音频流！");
    }

    // 创建视频解码线程
    m_videoDecodeThread.reset(static_cast<VideoDecodeThread*>(
        ThreadRegistry::instance().createThread(ThreadType::VideoDecode)));
    if (m_videoDecodeThread) {
        m_videoDecodeThread->setDecoder(m_videoDecoder.get());
        m_videoDecodeThread->setOutputQueue(m_videoFrameQueue.get());
        connect(m_videoDecodeThread.get(), &VideoDecodeThread::sigFrameDecoded,
                this, &PlayEngine::onFrameDecoded);
    }

    // 创建音频解码线程
    m_audioDecodeThread.reset(static_cast<AudioDecodeThread*>(
        ThreadRegistry::instance().createThread(ThreadType::AudioDecode)));
    if (m_audioDecodeThread) {
        m_audioDecodeThread->setDecoder(m_audioDecoder.get());
        m_audioDecodeThread->setOutputQueue(m_audioFrameQueue.get());
        connect(m_audioDecodeThread.get(), &AudioDecodeThread::sigFrameDecoded,
                this, &PlayEngine::onAudioFrameDecoded);
    }

    // 创建渲染器
    if (m_renderWindowId) {
        m_renderer = std::make_unique<SDLRenderer>();
        m_renderer->init(m_renderWindowId, m_width, m_height);
    }

    LOG_INFO("PlayEngine", "所有模块初始化完成");
}