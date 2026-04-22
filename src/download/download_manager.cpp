#include "download_manager.h"
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include "src/common/logger.h"

DownloadManager& DownloadManager::instance()
{
    static DownloadManager instance;
    return instance;
}

DownloadManager::DownloadManager()
{
    LOG_INFO("DownloadManager", "Constructor started");

    m_nam = new QNetworkAccessManager(this);
    if (m_nam) {
        LOG_DEBUG("DownloadManager", "QNetworkAccessManager created successfully");
    } else {
        LOG_ERROR("DownloadManager", "Failed to create QNetworkAccessManager");
    }

    m_speedTimer = new QTimer(this);
    if (m_speedTimer) {
        m_speedTimer->setInterval(1000);
        connect(m_speedTimer, &QTimer::timeout, this, &DownloadManager::onUpdateSpeed);
        m_speedTimer->start();
        LOG_DEBUG("DownloadManager", "Speed timer started");
    }

    m_defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    LOG_DEBUG("DownloadManager", "Default path: " + m_defaultPath);

    loadTaskList();
    LOG_INFO("DownloadManager", "Constructor finished");
}

DownloadManager::~DownloadManager()
{
    LOG_INFO("DownloadManager", "Shutting down, saving task list...");
    saveTaskList();
}

int DownloadManager::generateTaskId()
{
    static int s_nextId = 1;
    return s_nextId++;
}

void DownloadManager::startNextTask()
{
    // 使用最原始的控制台输出，不依赖任何对象
    printf("DOWNLOAD_DEBUG: startNextTask ENTER\n");
    fflush(stdout);
    return;
}

void DownloadManager::addDownload(const QString& url, const QString& savePath)
{
    printf("!!! VERSION 2 - addDownload ENTER !!!\n");
    fflush(stdout);

    LOG_INFO("DownloadManager", "addDownload called");


    // 清理URL：去除首尾空格、换行符、引号
    QString cleanUrl = url.trimmed();
    if (cleanUrl.startsWith('"') && cleanUrl.endsWith('"')) {
        cleanUrl = cleanUrl.mid(1, cleanUrl.length() - 2);
        LOG_DEBUG("DownloadManager", "Removed quotes from URL");
    }
    cleanUrl = cleanUrl.trimmed();

    LOG_DEBUG("DownloadManager", "  Original URL: " + url);
    LOG_DEBUG("DownloadManager", "  Cleaned URL: " + cleanUrl);
    LOG_DEBUG("DownloadManager", "  SavePath: " + savePath);

    // 安全检查（使用清理后的 cleanUrl）
    if (cleanUrl.isEmpty()) {
        LOG_ERROR("DownloadManager", "URL is empty after cleaning");
        return;
    }

    if (!cleanUrl.startsWith("http")) {
        LOG_ERROR("DownloadManager", "Invalid url (not http): " + cleanUrl.left(50));
        return;
    }

    DownloadTask task;
    task.id = generateTaskId();
    task.url = cleanUrl;  // 使用清理后的 URL
    task.status = 0;

    // 获取文件名（从清理后的 URL）
    QString fileName = QFileInfo(cleanUrl).fileName();
    if (fileName.isEmpty() || !fileName.contains(".")) {
        fileName = "download_" + QString::number(task.id) + ".mp4";
        LOG_WARN("DownloadManager", "Generated filename: " + fileName);
    }
    task.fileName = fileName;

    // 确定保存路径
    QString finalPath = savePath;
    if (finalPath.isEmpty()) {
        finalPath = m_defaultPath;
        LOG_DEBUG("DownloadManager", "Using default path: " + finalPath);
    }
    if (finalPath.isEmpty()) {
        finalPath = QDir::currentPath();
        LOG_WARN("DownloadManager", "Fallback to current path: " + finalPath);
    }

    // 确保目录存在
    QDir dir(finalPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("DownloadManager", "Failed to create directory: " + finalPath);
            return;
        }
        LOG_DEBUG("DownloadManager", "Created directory: " + finalPath);
    }

    task.savePath = finalPath + "/" + fileName;

    LOG_INFO("DownloadManager", "Task added: id=" + QString::number(task.id) +
                                    ", file=" + task.fileName + ", path=" + task.savePath);
    LOG_DEBUG("DownloadManager", "Task status=" + QString::number(task.status));
    LOG_DEBUG("DownloadManager", "Calling startNextTask...");

    m_tasks.append(task);
    emit taskAdded(task.id);
    saveTaskList();

    qDebug() << "=== MANAGER: BEFORE startNextTask ===";
    printf("MANAGER: BEFORE startNextTask\n");
    fflush(stdout);

    startNextTask();

    qDebug() << "=== MANAGER: AFTER startNextTask ===";
    printf("MANAGER: AFTER startNextTask\n");
    fflush(stdout);

    LOG_DEBUG("DownloadManager", "addDownload finished");
}



void DownloadManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    // 找到对应的任务
    for (auto& task : m_tasks) {
        if (m_activeReplies.contains(task.id)) {
            task.downloadedSize = bytesReceived;
            task.totalSize = bytesTotal;
            if (bytesTotal > 0) {
                task.progress = static_cast<int>(bytesReceived * 100 / bytesTotal);
            }
            updateTask(task);
            break;
        }
    }
}

void DownloadManager::onReadyRead()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        LOG_WARN("DownloadManager", "onReadyRead: sender is not QNetworkReply");
        return;
    }

    int taskId = m_activeReplies.key(reply);
    if (m_activeFiles.contains(taskId)) {
        QFile* file = m_activeFiles[taskId];
        if (file && file->isOpen()) {
            file->write(reply->readAll());
        } else {
            LOG_ERROR("DownloadManager", "File not open for task " + QString::number(taskId));
        }
    }
}

void DownloadManager::onDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    int taskId = m_activeReplies.key(reply);
    LOG_INFO("DownloadManager", "Download finished for task " + QString::number(taskId));

    if (reply->error() == QNetworkReply::NoError) {
        // 下载成功
        for (auto& task : m_tasks) {
            if (task.id == taskId) {
                task.status = 3;  // 完成
                task.progress = 100;
                updateTask(task);
                LOG_INFO("DownloadManager", "Task " + QString::number(taskId) + " completed successfully");
                break;
            }
        }
    } else {
        // 下载失败
        for (auto& task : m_tasks) {
            if (task.id == taskId) {
                task.status = 4;  // 错误
                task.errorMsg = reply->errorString();
                updateTask(task);
                LOG_ERROR("DownloadManager", "Task " + QString::number(taskId) +
                                                 " failed: " + task.errorMsg);
                break;
            }
        }
    }

    // 清理
    if (m_activeFiles.contains(taskId)) {
        QFile* file = m_activeFiles[taskId];
        if (file) {
            file->close();
            delete file;
        }
        m_activeFiles.remove(taskId);
    }
    m_activeReplies.remove(taskId);
    reply->deleteLater();

    // 开始下一个任务
    startNextTask();
}

void DownloadManager::onUpdateSpeed()
{
    static QMap<int, qint64> lastBytes;

    for (auto& task : m_tasks) {
        if (task.status == 1 && m_activeReplies.contains(task.id)) {
            qint64 current = task.downloadedSize;
            if (lastBytes.contains(task.id)) {
                task.speed = (current - lastBytes[task.id]) / 1024;  // KB/s
            }
            lastBytes[task.id] = current;
            updateTask(task);
        }
    }
}

void DownloadManager::pauseTask(int taskId)
{
    LOG_INFO("DownloadManager", "Pausing task " + QString::number(taskId));

    for (auto& task : m_tasks) {
        if (task.id == taskId && task.status == 1) {
            task.status = 2;  // 暂停
            updateTask(task);

            if (m_activeReplies.contains(taskId)) {
                m_activeReplies[taskId]->abort();
                LOG_DEBUG("DownloadManager", "Task " + QString::number(taskId) + " aborted");
            }
            break;
        }
    }
    startNextTask();
}

void DownloadManager::resumeTask(int taskId)
{
    LOG_INFO("DownloadManager", "Resuming task " + QString::number(taskId));

    for (auto& task : m_tasks) {
        if (task.id == taskId && task.status == 2) {
            task.status = 0;  // 等待中
            updateTask(task);
            startNextTask();
            break;
        }
    }
}

void DownloadManager::removeTask(int taskId, bool deleteFile)
{
    LOG_INFO("DownloadManager", "Removing task " + QString::number(taskId) +
                                    ", deleteFile=" + (deleteFile ? "true" : "false"));

    for (int i = 0; i < m_tasks.size(); i++) {
        if (m_tasks[i].id == taskId) {
            // 如果正在下载，先取消
            if (m_activeReplies.contains(taskId)) {
                m_activeReplies[taskId]->abort();
            }
            // 删除文件
            if (deleteFile && m_tasks[i].status == 3) {
                if (QFile::remove(m_tasks[i].savePath)) {
                    LOG_DEBUG("DownloadManager", "Deleted file: " + m_tasks[i].savePath);
                } else {
                    LOG_WARN("DownloadManager", "Failed to delete file: " + m_tasks[i].savePath);
                }
            }
            m_tasks.removeAt(i);
            emit taskRemoved(taskId);
            break;
        }
    }
    saveTaskList();
    startNextTask();
}

void DownloadManager::retryTask(int taskId)
{
    LOG_INFO("DownloadManager", "Retrying task " + QString::number(taskId));

    for (auto& task : m_tasks) {
        if (task.id == taskId && task.status == 4) {
            // 重置任务状态
            task.status = 0;
            task.progress = 0;
            task.downloadedSize = 0;
            task.errorMsg.clear();
            updateTask(task);

            LOG_DEBUG("DownloadManager", "Task " + QString::number(taskId) + " reset, waiting to retry");

            // 重新开始下载
            startNextTask();
            break;
        }
    }
}

void DownloadManager::updateTask(DownloadTask& task)
{
    emit taskUpdated(task.id);
    saveTaskList();
}

void DownloadManager::saveTaskList()
{
    QJsonArray arr;
    for (const auto& task : m_tasks) {
        QJsonObject obj;
        obj["id"] = task.id;
        obj["url"] = task.url;
        obj["fileName"] = task.fileName;
        obj["savePath"] = task.savePath;
        obj["totalSize"] = static_cast<qint64>(task.totalSize);
        obj["downloadedSize"] = static_cast<qint64>(task.downloadedSize);
        obj["status"] = task.status;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile file("download_tasks.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        LOG_DEBUG("DownloadManager", "Saved " + QString::number(m_tasks.size()) + " tasks");
    } else {
        LOG_WARN("DownloadManager", "Failed to save task list");
    }
}

void DownloadManager::loadTaskList()
{
    QFile file("download_tasks.json");
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_DEBUG("DownloadManager", "No existing task file found");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray arr = doc.array();

    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        DownloadTask task;
        task.id = obj["id"].toInt();
        task.url = obj["url"].toString();
        task.fileName = obj["fileName"].toString();
        task.savePath = obj["savePath"].toString();
        task.totalSize = obj["totalSize"].toVariant().toLongLong();
        task.downloadedSize = obj["downloadedSize"].toVariant().toLongLong();
        task.status = obj["status"].toInt();

        // 未完成的任务重置为暂停状态
        if (task.status == 1) {
            task.status = 2;
        }

        m_tasks.append(task);
    }

    LOG_INFO("DownloadManager", "Loaded " + QString::number(m_tasks.size()) + " existing tasks");
}