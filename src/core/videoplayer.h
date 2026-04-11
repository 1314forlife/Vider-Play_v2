#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>  // ✅ 新增：需要包含这个
#include "irender.h"
#include "ffdecoder.h"
#include "frame_data.h"

class VideoPlayer : public QThread
{
    Q_OBJECT
public:
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer() override;

    void setRender(IRender* render);
    bool openFile(const QString& filePath);
    void play();
    void pause();
    void stop();
    void resume();

    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    bool isStoped() const { return m_isStopped; }

signals:
    void sigPlayStateChanged(bool isPlaying);
    void sigPlayFinished();
    void sigRenderFrame(const FrameData& frame);

protected:
    void run() override;

private:
    IRender* m_render = nullptr;
    FFDecoder m_decoder;

    // 线程同步
    mutable QMutex m_mutex;        // 改为 mutable，允许 const 方法中锁定
    QWaitCondition m_pauseCond;    // ✅ 新增：用于暂停等待

    // 状态标志
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_isStopped = true;
};

#endif // VIDEOPLAYER_H