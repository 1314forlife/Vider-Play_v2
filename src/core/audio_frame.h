#ifndef AUDIO_FRAME_H
#define AUDIO_FRAME_H

#include <memory>
#include <cstdint>
#include <cstring>

extern "C" {
#include <libavutil/frame.h>
}

class AudioFrame {
public:
    AudioFrame() = default;

    AudioFrame(AudioFrame&& other) noexcept = default;
    AudioFrame& operator=(AudioFrame&& other) noexcept = default;

    AudioFrame(const AudioFrame&) = delete;
    AudioFrame& operator=(const AudioFrame&) = delete;

    static AudioFrame fromAVFrame(AVFrame* frame, int64_t pts) {
        AudioFrame data;
        data.m_frame = std::shared_ptr<AVFrame>(frame, [](AVFrame* f) {
            if (f) av_frame_free(&f);
        });
        data.m_pts = pts;
        data.m_sampleRate = frame->sample_rate;
        data.m_channels = frame->ch_layout.nb_channels;
        data.m_samples = frame->nb_samples;
        data.m_format = frame->format;

        return data;
    }

    uint8_t** data() const { return m_frame ? m_frame->data : nullptr; }
    int samples() const { return m_samples; }
    int sampleRate() const { return m_sampleRate; }
    int channels() const { return m_channels; }
    int format() const { return m_format; }
    int64_t pts() const { return m_pts; }
    bool isValid() const { return m_frame != nullptr && m_samples > 0; }

private:
    std::shared_ptr<AVFrame> m_frame;
    int m_sampleRate = 0;
    int m_channels = 0;
    int m_samples = 0;
    int m_format = 0;
    int64_t m_pts = 0;
};

#endif