#ifndef VIDEO_SYNC_H
#define VIDEO_SYNC_H

#include "audio_clock.h"

class VideoSync {
public:
    VideoSync(AudioClock* audioClock) : m_audioClock(audioClock) {}

    // 计算下一帧应该等待的时间（微秒）
    int64_t calculateWaitTime(int64_t videoPts) {
        double videoTime = videoPts / 1000000.0;
        double audioTime = m_audioClock->getCurrentTime();

        double diff = videoTime - audioTime;

        // 动态阈值
        double threshold = m_audioClock->getSyncThreshold();

        if (diff > threshold) {
            // 视频太慢，等待
            return (int64_t)(diff * 1000000);
        } else if (diff < -threshold) {
            // 视频太快，丢帧
            m_shouldDrop = true;
            return 0;
        }

        m_shouldDrop = false;
        return 0;
    }

    bool shouldDropFrame() const { return m_shouldDrop; }

private:
    AudioClock* m_audioClock = nullptr;
    bool m_shouldDrop = false;
};

#endif