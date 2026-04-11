#ifndef FFDECODER_H
#define FFDECODER_H

// 🔴 第一步：彻底取消Qt冲突宏（必须放在最最开头！）
#undef slots
#undef signals
#undef emit

// 标准库类型（解决uint8_t、max_align_t识别问题）
#include <cstdint>
#include <cstddef>

// Qt头文件（宏恢复后再包含，避免冲突）
#include <QString>
#define slots Q_SLOTS
#define signals Q_SIGNALS
#define emit Q_EMIT

// FFmpeg头文件（宏已取消，安全包含）
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>



class FFDecoder
{
public:
    FFDecoder();
    ~FFDecoder();

    bool open(const QString& url);

    // 读取一帧解码后的YUV420P数据
    bool readFrame(uint8_t* data[], int linesize[], int64_t& pts);

    // 获取视频信息
    int width() const { return m_videoWidth; }
    int height() const { return m_videoHeight; }
    AVRational timeBase() const { return m_videoTimeBase; }
    double fps() const { return m_fps; }

    // 关闭文件，释放资源
    void close();

private:
    AVFormatContext* m_formatCtx = nullptr;  // 格式上下文
    AVCodecContext* m_codecCtx = nullptr;    // 解码器上下文
    AVFrame* m_frame = nullptr;              // 原始解码帧
    AVFrame* m_frameYuv = nullptr;           // 转换后的YUV420P帧
    SwsContext* m_swsCtx = nullptr;          // 格式转换上下文
    uint8_t* m_yuvBuffer = nullptr;          // YUV数据缓冲区

    int m_videoStreamIndex = -1;             // 视频流索引
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    AVRational m_videoTimeBase = {0, 1};     // 视频时间基
    double m_fps = 0.0;                      // 视频帧率
};

#endif // FFDECODER_H
