#ifndef AUDIO_CLOCK_H
#define AUDIO_CLOCK_H

#include <atomic>
#include <chrono>

class AudioClock {
public:
    AudioClock() = default;

    // 更新时钟（每解码一帧音频）
    void update(int64_t pts, int sampleRate, int channels, int samples) {
        m_pts = pts;
        m_sampleRate = sampleRate;
        m_channels = channels;
        m_samplesPerFrame = samples;
        m_lastUpdateTime = std::chrono::steady_clock::now();
    }

    // 获取当前时钟（考虑已播放时间）
    double getCurrentTime() {
        if (m_sampleRate == 0) return 0.0;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - m_lastUpdateTime).count();

        // 计算已播放的时长
        double playedDuration = (double)m_playedSamples / m_sampleRate;

        return m_pts / 1000000.0 + playedDuration + elapsed;
    }

    // 通知已播放的样本数（由音频回调调用）
    void onSamplesPlayed(int samples) {
        m_playedSamples += samples;
    }

    // 同步阈值（动态调整）
    double getSyncThreshold() {
        // 根据帧率动态调整
        double base = 0.04;  // 40ms 基础阈值
        return base;
    }

    void reset() {
        m_pts = 0;
        m_playedSamples = 0;
        m_sampleRate = 0;
    }

private:
    std::atomic<int64_t> m_pts{0};
    std::atomic<int> m_playedSamples{0};
    std::atomic<int> m_sampleRate{0};
    std::atomic<int> m_channels{0};
    std::atomic<int> m_samplesPerFrame{0};
    std::chrono::steady_clock::time_point m_lastUpdateTime;
};

#endif