#ifndef MODULE_REGISTRY_H
#define MODULE_REGISTRY_H

#include <QMap>
#include <QString>
#include <functional>
#include <memory>

template<typename T>
class ModuleRegistry {
public:
    using Factory = std::function<std::unique_ptr<T>()>;

    static ModuleRegistry& instance() {
        static ModuleRegistry registry;
        return registry;
    }

    void registerModule(const QString& name, Factory factory) {
        m_factories[name] = factory;
    }

    std::unique_ptr<T> create(const QString& name) {
        if (m_factories.contains(name)) {
            return m_factories[name]();
        }
        return nullptr;
    }

    QStringList getModuleNames() const {
        return m_factories.keys();
    }

    bool hasModule(const QString& name) const {
        return m_factories.contains(name);
    }

private:
    QMap<QString, Factory> m_factories;
};

#endif