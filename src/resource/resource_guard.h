#ifndef RESOURCE_GUARD_H
#define RESOURCE_GUARD_H

#include <functional>

/**
 * 通用资源守卫
 * 用于自动管理任意类型的资源
 */
template<typename T>
class ResourceGuard {
public:
    using Deleter = std::function<void(T*)>;

    explicit ResourceGuard(T* resource = nullptr, Deleter deleter = nullptr)
        : m_resource(resource), m_deleter(deleter) {}

    ~ResourceGuard() {
        release();
    }

    // 禁止拷贝
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;

    // 支持移动
    ResourceGuard(ResourceGuard&& other) noexcept
        : m_resource(other.m_resource), m_deleter(std::move(other.m_deleter)) {
        other.m_resource = nullptr;
    }

    ResourceGuard& operator=(ResourceGuard&& other) noexcept {
        if (this != &other) {
            release();
            m_resource = other.m_resource;
            m_deleter = std::move(other.m_deleter);
            other.m_resource = nullptr;
        }
        return *this;
    }

    // 获取原始指针
    T* get() const { return m_resource; }

    // 释放所有权
    T* release() {
        T* resource = m_resource;
        m_resource = nullptr;
        return resource;
    }

    // 重置资源
    void reset(T* resource = nullptr, Deleter deleter = nullptr) {
        release();
        m_resource = resource;
        if (deleter) {
            m_deleter = deleter;
        }
    }

    // 判断是否有效
    explicit operator bool() const { return m_resource != nullptr; }

    // 访问操作符
    T* operator->() const { return m_resource; }
    T& operator*() const { return *m_resource; }

private:
    void release() {
        if (m_resource && m_deleter) {
            m_deleter(m_resource);
        }
        m_resource = nullptr;
    }

    T* m_resource = nullptr;
    Deleter m_deleter;
};

/**
 * 作用域守卫
 * 在离开作用域时自动执行清理函数
 */
class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> cleanup)
        : m_cleanup(cleanup), m_active(true) {}

    ~ScopeGuard() {
        if (m_active && m_cleanup) {
            m_cleanup();
        }
    }

    void dismiss() { m_active = false; }

    // 禁止拷贝
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // 支持移动
    ScopeGuard(ScopeGuard&& other) noexcept
        : m_cleanup(std::move(other.m_cleanup)), m_active(other.m_active) {
        other.dismiss();
    }

private:
    std::function<void()> m_cleanup;
    bool m_active = true;
};

#endif // RESOURCE_GUARD_H