#ifndef RING_QUEUE_H
#define RING_QUEUE_H

#include <atomic>
#include <vector>
#include <cstddef>

/**
 * 无锁环形队列
 * 适用于单生产者单消费者场景
 * 使用原子操作，无互斥锁
 */
template<typename T>
class RingQueue {
public:
    explicit RingQueue(size_t capacity)
        : m_capacity(capacity), m_buffer(capacity), m_readPos(0), m_writePos(0) {}

    // 禁止拷贝
    RingQueue(const RingQueue&) = delete;
    RingQueue& operator=(const RingQueue&) = delete;

    // 生产者：写入数据
    bool push(T&& item) {
        size_t write = m_writePos.load(std::memory_order_relaxed);
        size_t next = (write + 1) % m_capacity;

        if (next == m_readPos.load(std::memory_order_acquire)) {
            return false;  // 队列满
        }

        m_buffer[write] = std::move(item);
        m_writePos.store(next, std::memory_order_release);
        return true;
    }

    // 生产者：尝试写入（带等待）
    bool pushWait(T&& item, int maxRetries = 100) {
        for (int i = 0; i < maxRetries; i++) {
            if (push(std::move(item))) {
                return true;
            }
            // 队列满，让出CPU
            std::this_thread::yield();
        }
        return false;
    }

    // 消费者：读取数据
    bool pop(T& item) {
        size_t read = m_readPos.load(std::memory_order_relaxed);

        if (read == m_writePos.load(std::memory_order_acquire)) {
            return false;  // 队列空
        }

        item = std::move(m_buffer[read]);
        m_readPos.store((read + 1) % m_capacity, std::memory_order_release);
        return true;
    }

    // 消费者：尝试读取（带等待）
    bool popWait(T& item, int maxRetries = 100) {
        for (int i = 0; i < maxRetries; i++) {
            if (pop(item)) {
                return true;
            }
            // 队列空，让出CPU
            std::this_thread::yield();
        }
        return false;
    }

    // 清空队列
    void clear() {
        T dummy;
        while (pop(dummy));
    }

    // 获取队列大小（近似值，不是精确值）
    size_t size() const {
        size_t write = m_writePos.load(std::memory_order_acquire);
        size_t read = m_readPos.load(std::memory_order_acquire);
        if (write >= read) {
            return write - read;
        } else {
            return m_capacity - read + write;
        }
    }

    bool empty() const {
        return m_readPos.load(std::memory_order_acquire) ==
               m_writePos.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t write = m_writePos.load(std::memory_order_acquire);
        size_t next = (write + 1) % m_capacity;
        return next == m_readPos.load(std::memory_order_acquire);
    }

    size_t capacity() const { return m_capacity - 1; }

private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    std::atomic<size_t> m_readPos;
    std::atomic<size_t> m_writePos;
};

#endif