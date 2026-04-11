#include "videoplayer.h"
#include <QDebug>
#include <QElapsedTimer>

VideoPlayer::VideoPlayer(QObject *parent)
    : QThread(parent)
{
}

VideoPlayer::~VideoPlayer()
{
    stop();
    wait();
}

void VideoPlayer::setRender(IRender *render)
{
    QMutexLocker locker(&m_mutex);
    m_render = render;
}

bool VideoPlayer::openFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    return m_decoder.open(filePath);
}

void VideoPlayer::play()
{
    QMutexLocker locker(&m_mutex);

    if (m_isStopped) {
        // 全新播放
        m_isStopped = false;
        m_isPaused = false;
        m_isPlaying = true;

        if (!isRunning()) {
            start();
        }
    } else if (m_isPaused) {
        // 恢复播放
        m_isPaused = false;
        m_isPlaying = true;
        m_pauseCond.wakeOne();  // ✅ 唤醒渲染线程
        qDebug() << "Resumed";
    }

    emit sigPlayStateChanged(true);
}

void VideoPlayer::pause()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isPaused && !m_isStopped) {
        m_isPaused = true;
        m_isPlaying = false;
        qDebug() << "Paused";
        emit sigPlayStateChanged(false);
    }
}

void VideoPlayer::stop()
{
    {
        QMutexLocker locker(&m_mutex);

        if (m_isStopped) {
            return;  // 已经停止
        }

        m_isStopped = true;
        m_isPlaying = false;
        m_isPaused = false;

        // ✅ 唤醒可能在等待的线程
        m_pauseCond.wakeAll();
    }

    wait();  // 等待线程结束
    m_decoder.close();
    emit sigPlayStateChanged(false);
    qDebug() << "Stopped";
}

void VideoPlayer::resume()
{
    // resume 和 play 逻辑相同，直接调用 play
    play();
}

void VideoPlayer::run()
{
    if (!m_render) {
        qDebug() << "Render not set";
        return;
    }

    m_render->setSize(m_decoder.width(), m_decoder.height());

    uint8_t* yuvData[4] = { nullptr };
    int linesize[4] = { 0 };
    int64_t pts = 0;

    QElapsedTimer frameTimer;
    frameTimer.start();
    double frameInterval = 1000.0 / m_decoder.fps();

    while (true) {
        // ============================================
        // 检查停止标志
        // ============================================
        {
            QMutexLocker locker(&m_mutex);
            if (m_isStopped) {
                break;
            }
        }

        // ============================================
        // ✅ 正确的暂停处理：使用条件变量等待
        // ============================================
        {
            QMutexLocker locker(&m_mutex);
            while (m_isPaused && !m_isStopped) {
                qDebug() << "Waiting for resume...";
                // 等待恢复信号，最长等待100ms（防止死锁）
                m_pauseCond.wait(&m_mutex, 100);
            }

            // 被唤醒后再次检查是否应该退出
            if (m_isStopped) {
                break;
            }
        }

        // ============================================
        // 读取帧数据
        // ============================================
        if (!m_decoder.readFrame(yuvData, linesize, pts)) {
            // 读不到帧时短暂休眠，避免CPU空转
            msleep(5);
            continue;
        }

        // ============================================
        // 帧间隔控制
        // ============================================
        qint64 elapsed = frameTimer.elapsed();
        if (elapsed < frameInterval) {
            msleep(frameInterval - elapsed);
        }

        // ============================================
        // 发送渲染信号
        // ============================================
        FrameData frame;
        frame.data = { yuvData[0], yuvData[1], yuvData[2] };
        frame.linesize = { linesize[0], linesize[1], linesize[2] };
        emit sigRenderFrame(frame);

        frameTimer.restart();
    }

    emit sigPlayFinished();
    qDebug() << "Play finished";
}