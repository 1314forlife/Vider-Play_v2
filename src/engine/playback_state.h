#ifndef PLAYBACK_STATE_H
#define PLAYBACK_STATE_H

enum class PlaybackState {
    Stopped,    // 停止
    Playing,    // 播放中
    Paused,     // 暂停
    Buffering   // 缓冲中
};

#endif