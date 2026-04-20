#include "logger.h"
#include <QDebug>

void Logger::setFile(const QString& path) {
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_file.setFileName(path);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "日志文件已创建:" << path;
    }
}

void Logger::log(LogLevel level, const QString& tag, const QString& msg) {
    if (level < m_level) return;

    QString prefix;
    switch (level) {
    case LOG_DEBUG: prefix = "[DEBUG]"; break;
    case LOG_INFO:  prefix = "[INFO] "; break;
    case LOG_WARN:  prefix = "[WARN] "; break;
    case LOG_ERROR: prefix = "[ERROR]"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString line = QString("%1 %2 [%3] %4").arg(timestamp, prefix, tag, msg);

    // 输出到控制台
    qDebug() << line;

    // 输出到文件
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.write((line + "\n").toUtf8());
        m_file.flush();
    }
}