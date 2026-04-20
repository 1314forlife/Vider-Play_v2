#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QDateTime>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void setLevel(LogLevel level) { m_level = level; }
    void setFile(const QString& path);

    void debug(const QString& tag, const QString& msg) { log(LOG_DEBUG, tag, msg); }
    void info(const QString& tag, const QString& msg)  { log(LOG_INFO,  tag, msg); }
    void warn(const QString& tag, const QString& msg)  { log(LOG_WARN,  tag, msg); }
    void error(const QString& tag, const QString& msg) { log(LOG_ERROR, tag, msg); }

private:
    Logger() = default;
    void log(LogLevel level, const QString& tag, const QString& msg);

    LogLevel m_level = LOG_DEBUG;
    QFile m_file;
    QMutex m_mutex;
};

// 便捷宏
#define LOG_DEBUG(tag, msg) Logger::instance().debug(tag, msg)
#define LOG_INFO(tag, msg)  Logger::instance().info(tag, msg)
#define LOG_WARN(tag, msg)  Logger::instance().warn(tag, msg)
#define LOG_ERROR(tag, msg) Logger::instance().error(tag, msg)

#endif