#ifndef DOWNLOAD_MANAGER_V2_H
#define DOWNLOAD_MANAGER_V2_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QQueue>

struct DownloadTaskV2 {
    int id;
    QString url;
    QString savePath;
    int progress;
    bool active;
};

class DownloadManagerV2 : public QObject
{
    Q_OBJECT

public:
    explicit DownloadManagerV2(QObject* parent = nullptr);

    // 添加下载任务
    int addDownload(const QString& url, const QString& savePath);

    // 控制任务
    void pauseTask(int taskId);
    void resumeTask(int taskId);
    void removeTask(int taskId);
    void removeTask(int taskId, bool deleteFile = false);
signals:
    void taskAdded(int taskId);
    void taskProgress(int taskId, int percent);
    void taskFinished(int taskId, bool success, const QString& errorMsg);
    void taskRemoved(int taskId);
    void taskSpeed(int taskId, int speedKBps);

private slots:
    void onReadyRead();
    void onFinished();
    void onProgress(qint64 received, qint64 total);
    void startNextTask();
    // ✅ 添加这一行


private:
    void cleanupCurrentTask();

    QNetworkAccessManager* m_nam;
    QNetworkReply* m_reply;
    QFile* m_file;

    QQueue<DownloadTaskV2> m_pendingTasks;
    DownloadTaskV2 m_currentTask;
    bool m_busy;

    int m_nextTaskId;

    qint64 m_pausedSize;      // 暂停时已下载的字节数
    bool m_isPaused;          // 是否处于暂停状态
};

#endif // DOWNLOAD_MANAGER_V2_H