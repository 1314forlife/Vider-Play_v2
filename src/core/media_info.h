#ifndef MEDIA_INFO_H
#define MEDIA_INFO_H

#include <QString>
#include <cstdint>

struct MediaInfo {
    QString filename;
    int width = 0;
    int height = 0;
    double fps = 25.0;
    int64_t duration = 0;  // 微秒
    int videoBitrate = 0;

    bool hasVideo = false;
    bool hasAudio = false;

    // 音频信息（后续扩展）
    int sampleRate = 0;
    int channels = 0;
};

#endif