#include "download_manager_v2.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>

DownloadManagerV2::DownloadManagerV2(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_file(nullptr)
    , m_busy(false)
    , m_nextTaskId(1)
{
    qDebug() << "[V2] DownloadManagerV2 created";
}

int DownloadManagerV2::addDownload(const QString& url, const QString& savePath)
{
    DownloadTaskV2 task;
    task.id = m_nextTaskId++;
    task.url = url;
    task.savePath = savePath;
    task.progress = 0;
    task.active = true;

    m_pendingTasks.enqueue(task);
    qDebug() << "[V2] Task added, id=" << task.id << ", queue size=" << m_pendingTasks.size();

    emit taskAdded(task.id);
    startNextTask();

    return task.id;
}

void DownloadManagerV2::startNextTask()
{
    if (m_busy) {
        qDebug() << "[V2] Busy, waiting...";
        return;
    }

    if (m_pendingTasks.isEmpty()) {
        qDebug() << "[V2] No pending tasks";
        return;
    }

    m_currentTask = m_pendingTasks.dequeue();
    m_busy = true;

    qDebug() << "[V2] Starting task id=" << m_currentTask.id << ", url=" << m_currentTask.url;

    // 确保目录存在
    QFileInfo info(m_currentTask.savePath);
    QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 打开文件
    m_file = new QFile(m_currentTask.savePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        qDebug() << "[V2] Failed to open file";
        emit taskFinished(m_currentTask.id, false, "无法创建文件");
        cleanupCurrentTask();
        startNextTask();
        return;
    }

    // 发起请求
    QUrl qurl(m_currentTask.url);
    QNetworkRequest request(qurl);
    m_reply = m_nam->get(request);

    if (!m_reply) {
        qDebug() << "[V2] Failed to create request";
        emit taskFinished(m_currentTask.id, false, "网络请求失败");
        cleanupCurrentTask();
        startNextTask();
        return;
    }

    connect(m_reply, &QNetworkReply::readyRead, this, &DownloadManagerV2::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &DownloadManagerV2::onFinished);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadManagerV2::onProgress);
}

void DownloadManagerV2::onReadyRead()
{
    if (m_reply && m_file) {
        m_file->write(m_reply->readAll());
    }
}

void DownloadManagerV2::onProgress(qint64 received, qint64 total)
{
    static QElapsedTimer timer;
    static qint64 lastReceived = 0;

    if (!timer.isValid()) {
        timer.start();
        lastReceived = received;
    }

    qint64 elapsed = timer.elapsed();
    if (elapsed >= 1000) {
        qint64 diff = received - lastReceived;
        int speedKBps = diff * 1000 / elapsed / 1024;  // KB/s
        emit taskSpeed(m_currentTask.id, speedKBps);

        // 重置
        lastReceived = received;
        timer.restart();
    }

    // 原有进度信号...
    int percent = total > 0 ? (received * 100 / total) : 0;
    emit taskProgress(m_currentTask.id, percent);
}

void DownloadManagerV2::onFinished()
{
    bool success = (m_reply->error() == QNetworkReply::NoError);
    QString errorMsg;

    if (success) {
        qDebug() << "[V2] Task" << m_currentTask.id << "completed!";
        if (m_file) {
            m_file->close();
        }
        // 确保发送 100% 进度
        emit taskProgress(m_currentTask.id, 100);
        emit taskFinished(m_currentTask.id, true, "");
    } else {
        errorMsg = m_reply->errorString();
        qDebug() << "[V2] Task" << m_currentTask.id << "failed:" << errorMsg;
        if (m_file) {
            m_file->close();
            m_file->remove();
        }
        emit taskFinished(m_currentTask.id, false, errorMsg);
    }

    cleanupCurrentTask();
    startNextTask();
}

void DownloadManagerV2::cleanupCurrentTask()
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file) {
        delete m_file;
        m_file = nullptr;
    }
    m_busy = false;
}

void DownloadManagerV2::pauseTask(int taskId)
{
    qDebug() << "[V2] Pause task" << taskId << " - TODO";
}

void DownloadManagerV2::resumeTask(int taskId)
{
    qDebug() << "[V2] Resume task" << taskId << " - TODO";
}

void DownloadManagerV2::removeTask(int taskId, bool deleteFile)
{
    qDebug() << "[V2] Removing task" << taskId << "deleteFile=" << deleteFile;

    // 如果是正在下载的任务
    if (m_currentTask.id == taskId && m_busy) {
        if (m_reply) {
            m_reply->abort();
        }
        if (m_file) {
            m_file->close();
            if (deleteFile) {
                m_file->remove();
            }
            delete m_file;
            m_file = nullptr;
        }
        m_busy = false;
        emit taskFinished(taskId, false, "用户取消");
    }

    // 从等待队列中移除
    QQueue<DownloadTaskV2> newQueue;
    for (const auto& task : m_pendingTasks) {
        if (task.id != taskId) {
            newQueue.enqueue(task);
        } else if (deleteFile) {
            QFile::remove(task.savePath);
        }
    }
    m_pendingTasks = newQueue;

    emit taskRemoved(taskId);
}