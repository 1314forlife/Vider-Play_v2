#include "src/stdafx.h"
#include "ffdecoder.h"
#include <QDebug>


#define CHECK_AV_ERROR(ret, action) \
do { \
        if ((ret) < 0) { \
            char errBuf[AV_ERROR_MAX_STRING_SIZE]; \
            av_strerror(ret, errBuf, sizeof(errBuf)); \
            qDebug() << action << "failed:" << errBuf; \
            return false; \
    } \
} while(0)

FFDecoder::FFDecoder()
{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
    avformat_network_init();
#endif
}

FFDecoder::~FFDecoder()
{
    close();
}

bool FFDecoder::open(const QString &url)
{
    int ret = avformat_open_input(&m_formatCtx,url.toUtf8().constData(),nullptr,nullptr);
    if (ret < 0) {
        CHECK_AV_ERROR(ret,"Open file failed:");
    }

    //2.读取流信息

    ret = avformat_find_stream_info(m_formatCtx,nullptr);
    if (ret < 0) {
        CHECK_AV_ERROR(ret,"Find stream info failed");
    }

    for(int i = 0 ;i < m_formatCtx->nb_streams;i++){
        if(m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            m_videoStreamIndex = i;
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        qDebug() << "No video stream found";
        return false;
    }

    AVStream* videoStream = m_formatCtx->streams[m_videoStreamIndex];
    m_videoTimeBase = videoStream->time_base;
    m_fps = av_q2d(videoStream->avg_frame_rate);

    //初始化解码器
    AVCodecParameters* codecPar = videoStream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        qDebug() << "Decoder not found";
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qDebug() << "Alloc codec context failed";
        return false;
    }

    ret = avcodec_parameters_to_context(m_codecCtx,codecPar);
    if (ret < 0) {
        CHECK_AV_ERROR(ret,"Copy codec parameters failed");
    }

    ret = avcodec_open2(m_codecCtx,codec,nullptr);
    if(ret < 0){
        CHECK_AV_ERROR(ret,"Open codec failed:");
    }

    m_videoWidth = m_codecCtx->width;
    m_videoHeight = m_codecCtx->height;

    m_frameYuv = av_frame_alloc();
    m_frame = av_frame_alloc();
    if (!m_frame || !m_frameYuv) {
        qDebug() << "Alloc frame failed";
        return false;
    }

    m_swsCtx = sws_getContext(
        m_videoWidth,m_videoHeight,m_codecCtx->pix_fmt,
        m_videoWidth,m_videoHeight,AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
        );

    // 分配YUV缓冲区
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_videoWidth, m_videoHeight, 1);
    m_yuvBuffer = new uint8_t[bufferSize];
    av_image_fill_arrays(m_frameYuv->data, m_frameYuv->linesize, m_yuvBuffer,
                         AV_PIX_FMT_YUV420P, m_videoWidth, m_videoHeight, 1);

    qDebug() << "Open file success! Resolution:" << m_videoWidth << "x" << m_videoHeight << "FPS:" << m_fps;
    return true;
}

bool FFDecoder::readFrame(uint8_t *data[], int linesize[], int64_t &pts)
{
    if (!m_formatCtx || !m_codecCtx) return false;

    AVPacket* pkt = av_packet_alloc();
    while(av_read_frame(m_formatCtx,pkt) >= 0){
        if(pkt->stream_index != m_videoStreamIndex){
            av_packet_unref(pkt);
            continue;
        }

        int ret = avcodec_send_packet(m_codecCtx,pkt);
        if(ret < 0){
            CHECK_AV_ERROR(ret,"send failed");
            av_packet_unref(pkt);
            continue;
        }

        // 从解码器读取帧
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        av_packet_unref(pkt);

        if(ret == 0){
            sws_scale(m_swsCtx,
                      m_frame->data,m_frame->linesize,0,m_videoHeight,
                      m_frameYuv->data, m_frameYuv->linesize
                      );

            // 返回YUV数据和PTS（用于音视频同步）
            data[0] = m_frameYuv->data[0];
            data[1] = m_frameYuv->data[1];
            data[2] = m_frameYuv->data[2];
            linesize[0] = m_frameYuv->linesize[0];
            linesize[1] = m_frameYuv->linesize[1];
            linesize[2] = m_frameYuv->linesize[2];
            pts = m_frame->pts;

            av_packet_free(&pkt);
            return true;
        }
    }
    av_packet_free(&pkt);
    return false; // 读取到文件末尾
}

void FFDecoder::close()
{
    // 按顺序释放所有资源（避免内存泄漏）
    if (m_yuvBuffer) delete[] m_yuvBuffer;
    if (m_swsCtx) sws_freeContext(m_swsCtx);
    if (m_frameYuv) av_frame_free(&m_frameYuv);
    if (m_frame) av_frame_free(&m_frame);
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
    if (m_formatCtx) avformat_close_input(&m_formatCtx);

    m_yuvBuffer = nullptr;
    m_swsCtx = nullptr;
    m_frameYuv = nullptr;
    m_frame = nullptr;
    m_codecCtx = nullptr;
    m_formatCtx = nullptr;
    m_videoStreamIndex = -1;
}