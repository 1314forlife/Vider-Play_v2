#ifndef DOWNLOAD_TASK_H
#define DOWNLOAD_TASK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>


struct DownloadTask {
    int id = 0;
    QString url;
    QString fileName;
    QString savePath;
    qint64 totalSize = 0;
    qint64 downloadedSize = 0;
    int progress = 0;
    int speed = 0;
    int status = 0;  // 0=等待 1=下载中 2=暂停 3=完成 4=错误
    QString errorMsg;

    // 获取剩余时间（秒）
    int remainingSeconds() const {
        if (speed <= 0) return 0;
        qint64 remaining = totalSize - downloadedSize;
        return remaining / speed / 1024;
    }

    // 格式化文件大小
    static QString formatSize(qint64 bytes) {
        if (bytes < 1024) return QString::number(bytes) + " B";
        if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / 1024.0 / 1024.0, 'f', 1) + " MB";
        return QString::number(bytes / 1024.0 / 1024.0 / 1024.0, 'f', 2) + " GB";
    }

    // 格式化速度
    static QString formatSpeed(int kbPerSec) {
        if (kbPerSec < 1024) return QString::number(kbPerSec) + " KB/s";
        return QString::number(kbPerSec / 1024.0, 'f', 1) + " MB/s";
    }

    // 获取状态文本
    QString getStatusText() const {
        switch (status) {
        case 0: return "等待中";
        case 1: return "下载中";
        case 2: return "已暂停";
        case 3: return "已完成";
        case 4: return "错误: " + errorMsg;
        default: return "未知";
        }
    }
};

#endif // DOWNLOAD_TASK_H