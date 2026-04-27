#include "download_manager_v2.h"
#include "download_history.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>

DownloadManagerV2::DownloadManagerV2(QObject* parent) : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_file(nullptr)
    , m_busy(false)
    , m_nextTaskId(1)
    , m_pausedSize(0)
    , m_isPaused(false)
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
    m_isPaused = false;
    m_pausedSize = 0;

    qDebug() << "[V2] Starting task id=" << m_currentTask.id << ", url=" << m_currentTask.url;

    QFileInfo info(m_currentTask.savePath);
    QDir dir = info.absoluteDir();
    if (!dir.exists())
        dir.mkpath(".");

    m_file = new QFile(m_currentTask.savePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        qDebug() << "[V2] Failed to open file";
        emit taskFinished(m_currentTask.id, false, "无法创建文件");
        cleanupCurrentTask();
        startNextTask();
        return;
    }

    QNetworkRequest request(QUrl(m_currentTask.url));
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
    if (m_reply && m_file)
        m_file->write(m_reply->readAll());
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
        int speedKBps = diff * 1000 / elapsed / 1024;
        emit taskSpeed(m_currentTask.id, speedKBps);

        lastReceived = received;
        timer.restart();
    }

    // ✅ 修复：恢复时进度要累加已下载部分
    qint64 totalSize = m_pausedSize + (total > 0 ? total : 0);
    qint64 currentSize = m_pausedSize + received;
    int percent = (totalSize > 0) ? (currentSize * 100 / totalSize) : 0;

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
        emit taskProgress(m_currentTask.id, 100);
        emit taskFinished(m_currentTask.id, true, "");

        // ✅ 保存到历史记录
        DownloadHistory::instance().addRecord(
            m_currentTask.url,
            QFileInfo(m_currentTask.savePath).fileName(),
            m_currentTask.savePath,
            m_file ? m_file->size() : 0
            );
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

void DownloadManagerV2::pauseTask(int taskId)
{
    if (m_currentTask.id == taskId && m_busy && !m_isPaused) {
        m_pausedSize = m_file ? m_file->size() : 0;
        if (m_reply) {
            disconnect(m_reply, nullptr, this, nullptr);
            m_reply->abort();
            m_reply->deleteLater();
            m_reply = nullptr;
        }
        m_isPaused = true;
        m_busy = false;
        qDebug() << "Task" << taskId << "paused at" << m_pausedSize << "bytes";
    }
}

void DownloadManagerV2::resumeTask(int taskId)
{
    if (m_currentTask.id == taskId && m_isPaused) {
        QNetworkRequest request(QUrl(m_currentTask.url));
        QString range = QString("bytes=%1-").arg(m_pausedSize);
        request.setRawHeader("Range", range.toUtf8());

        m_reply = m_nam->get(request);

        if (m_file) {
            m_file->close();
            delete m_file;
        }
        m_file = new QFile(m_currentTask.savePath);
        if (!m_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
            emit taskFinished(taskId, false, "无法打开文件");
            return;
        }

        connect(m_reply, &QNetworkReply::readyRead, this, &DownloadManagerV2::onReadyRead);
        connect(m_reply, &QNetworkReply::finished, this, &DownloadManagerV2::onFinished);
        connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadManagerV2::onProgress);

        m_busy = true;
        m_isPaused = false;
        qDebug() << "Task" << taskId << "resumed from" << m_pausedSize;
    }
}

void DownloadManagerV2::removeTask(int taskId, bool deleteFile)
{
    qDebug() << "[V2] Removing task" << taskId << "deleteFile=" << deleteFile;

    if (m_currentTask.id == taskId && m_busy) {
        if (m_reply) {
            m_reply->abort();
            m_reply->deleteLater();
            m_reply = nullptr;
        }
        if (m_file) {
            m_file->close();
            if (deleteFile)
                m_file->remove();
            delete m_file;
            m_file = nullptr;
        }
        m_busy = false;
        emit taskFinished(taskId, false, "用户取消");
    }

    QQueue<DownloadTaskV2> newQueue;
    for (const auto& task : m_pendingTasks) {
        if (task.id != taskId)
            newQueue.enqueue(task);
        else if (deleteFile)
            QFile::remove(task.savePath);
    }
    m_pendingTasks = newQueue;

    emit taskRemoved(taskId);
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
    m_isPaused = false;
    m_pausedSize = 0;
}