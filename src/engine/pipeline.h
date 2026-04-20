#ifndef PIPELINE_H
#define PIPELINE_H

#include <QList>
#include <QString>
#include <functional>
#include "src/common/logger.h"

// 处理节点接口（不使用 Q_OBJECT，因为模板类不支持）
template<typename T>
class PipelineNode {
public:
    virtual ~PipelineNode() = default;
    virtual bool process(const T& input, T& output) = 0;
    virtual QString name() const = 0;
};

// 处理链
template<typename T>
class Pipeline {
public:
    using Processor = std::function<bool(const T&, T&)>;

    void addProcessor(Processor processor, const QString& name) {
        m_processors.append({processor, name});
    }

    bool process(const T& input, T& output) {
        T current = input;
        for (auto& proc : m_processors) {
            if (!proc.processor(current, current)) {
                LOG_ERROR("Pipeline", "处理失败: " << proc.name);
                return false;
            }
        }
        output = std::move(current);
        return true;
    }

    void clear() {
        m_processors.clear();
    }

    int size() const {
        return m_processors.size();
    }

private:
    struct ProcessorItem {
        Processor processor;
        QString name;
    };
    QList<ProcessorItem> m_processors;
};

#endif