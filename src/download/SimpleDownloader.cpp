#include "SimpleDownloader.h"
#include <QNetworkRequest>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

SimpleDownloader::SimpleDownloader(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_file(nullptr)
{
    qDebug() << "SimpleDownloader created";
}

void SimpleDownloader::download(const QString& url, const QString& savePath)
{
    qDebug() << "Downloading:" << url;
    qDebug() << "Save to:" << savePath;

    // 确保目录存在
    QFileInfo info(savePath);
    QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 打开文件
    m_file = new QFile(savePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file:" << savePath;
        emit finished(false, "无法创建文件");
        delete m_file;
        m_file = nullptr;
        return;
    }

    // 发起网络请求
    QUrl qurl(url);
    QNetworkRequest request(qurl);
    m_reply = m_nam->get(request);
    // 连接信号
    connect(m_reply, &QNetworkReply::readyRead, this, &SimpleDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &SimpleDownloader::onFinished);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &SimpleDownloader::onProgress);
}

void SimpleDownloader::onReadyRead()
{
    if (m_reply && m_file) {
        m_file->write(m_reply->readAll());
    }
}

void SimpleDownloader::onProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        int percent = (received * 100) / total;
        emit progress(percent);
        qDebug() << "Progress:" << percent << "%";
    }
}

void SimpleDownloader::onFinished()
{
    bool success = (m_reply->error() == QNetworkReply::NoError);
    QString errorMsg;

    if (success) {
        qDebug() << "Download completed!";
        if (m_file) {
            m_file->close();
        }
    } else {
        errorMsg = m_reply->errorString();
        qDebug() << "Download failed:" << errorMsg;
        if (m_file) {
            m_file->close();
            m_file->remove(); // 删除不完整的文件
        }
    }

    // 清理
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file) {
        delete m_file;
        m_file = nullptr;
    }

    emit finished(success, errorMsg);
}