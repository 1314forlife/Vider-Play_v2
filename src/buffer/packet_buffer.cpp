#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "core/packet.h"

class PacketBuffer {
public:
    PacketBuffer()
        : m_minSize(50), m_maxSize(500), m_targetSize(200) {}

    // 动态调整缓冲大小
    void push(Packet&& packet) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // 智能缓冲：根据网络/解码速度调整
        adjustBufferSize();

        // 防止溢出
        if (m_queue.size() >= m_maxSize) {
            // 丢弃最旧的包
            m_queue.pop_front();
            m_droppedCount++;
        }

        m_queue.push_back(std::move(packet));
        m_cond.notify_one();
    }

    bool pop(Packet& packet, int timeoutMs = 0) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (timeoutMs > 0) {
            auto deadline = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(timeoutMs);
            if (!m_cond.wait_until(lock, deadline, [this] { return !m_queue.empty(); })) {
                return false;
            }
        } else {
            m_cond.wait(lock, [this] { return !m_queue.empty(); });
        }

        packet = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
    }

    // 缓冲监控
    float getFillRatio() const {
        return (float)size() / m_targetSize;
    }

    int getDroppedCount() const { return m_droppedCount; }

private:
    void adjustBufferSize() {
        // 根据队列变化动态调整
        size_t currentSize = m_queue.size();

        if (currentSize > m_targetSize + 100) {
            // 堆积过多，增加消费速度
            m_targetSize = std::max(m_minSize, (int)(m_targetSize * 0.9));
        } else if (currentSize < m_targetSize - 50) {
            // 缓冲不足，增加缓冲
            m_targetSize = std::min(m_maxSize, (int)(m_targetSize * 1.1));
        }
    }

    std::deque<Packet> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;

    int m_minSize;
    int m_maxSize;
    int m_targetSize;
    std::atomic<int> m_droppedCount{0};
};

#endif