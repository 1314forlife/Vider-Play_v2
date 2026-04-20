// src/common/frame_queue.h
#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include "src/core/frame.h"

class FrameQueue {
public:
    explicit FrameQueue(size_t maxSize = 30)
        : m_maxSize(maxSize), m_frames(maxSize), m_rindex(0), m_windex(0), m_size(0) {}

    // 写入帧（阻塞直到有空间）
    bool push(FrameData&& frame, int timeoutMs = 0);

    // 读取帧（阻塞直到有数据）
    bool pop(FrameData& frame, int timeoutMs = 0);

    // 非阻塞读取
    bool tryPop(FrameData& frame);

    void clear();
    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    bool full() const { return m_size == m_maxSize; }

private:
    std::vector<FrameData> m_frames;
    size_t m_maxSize;
    size_t m_rindex;
    size_t m_windex;
    size_t m_size;

    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
};

#endif