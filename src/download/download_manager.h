#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>
#include <QList>
#include "download_task.h"

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    static DownloadManager& instance();

    // 添加下载任务
    void addDownload(const QString& url, const QString& savePath = "");

    // 控制任务
    void pauseTask(int taskId);
    void resumeTask(int taskId);
    void removeTask(int taskId, bool deleteFile = false);
    void retryTask(int taskId);

    // 获取任务列表
    QList<DownloadTask> getTasks() const { return m_tasks; }

signals:
    void taskAdded(int taskId);
    void taskUpdated(int taskId);
    void taskRemoved(int taskId);
    void allTasksChanged();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onReadyRead();
    void onUpdateSpeed();

private:
    DownloadManager();
    ~DownloadManager();

    int generateTaskId();
    void startNextTask();

    void updateTask(DownloadTask& task);
    void saveTaskList();
    void loadTaskList();

    QNetworkAccessManager* m_nam;
    QTimer* m_speedTimer;

    QList<DownloadTask> m_tasks;
    QMap<int, QNetworkReply*> m_activeReplies;
    QMap<int, QFile*> m_activeFiles;

    int m_maxConcurrent = 3;  // 最大并发数
    QString m_defaultPath;
};

#endif // DOWNLOAD_MANAGER_H