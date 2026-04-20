#include "network_player.h"
#include <QDebug>

NetworkPlayer::NetworkPlayer(QObject* parent) : QObject(parent) {
    m_ytProcess = new QProcess(this);
    m_ffmpegProcess = new QProcess(this);

    connect(m_ytProcess, &QProcess::finished, this, &NetworkPlayer::onYtFinished);
    connect(m_ytProcess, &QProcess::errorOccurred, this, &NetworkPlayer::onYtError);
}

NetworkPlayer::~NetworkPlayer() {
    stop();
}

void NetworkPlayer::play(const QString& url) {
    stop();
    m_currentUrl = url;

    // 用 yt-dlp 获取视频和音频地址
    QStringList args;
    args << "-g" << url;

    m_ytProcess->start("yt-dlp", args);
}

void NetworkPlayer::stop() {
    if (m_ytProcess && m_ytProcess->state() != QProcess::NotRunning) {
        m_ytProcess->kill();
    }
    if (m_ffmpegProcess && m_ffmpegProcess->state() != QProcess::NotRunning) {
        m_ffmpegProcess->kill();
    }
    if (m_ffmpegProcess) {
        m_ffmpegProcess->waitForFinished(1000);
    }
}

void NetworkPlayer::onYtFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0) {
        emit sigError("解析视频地址失败");
        return;
    }

    QString output = m_ytProcess->readAllStandardOutput();
    QStringList urls = output.split("\n", Qt::SkipEmptyParts);

    if (urls.size() < 2) {
        emit sigError("无法获取音视频地址");
        return;
    }

    QString videoUrl = urls[0].trimmed();
    QString audioUrl = urls[1].trimmed();

    qDebug() << "视频地址:" << videoUrl;
    qDebug() << "音频地址:" << audioUrl;

    // 用 FFmpeg 实时合并并输出到管道
    QStringList ffmpegArgs;
    ffmpegArgs << "-i" << videoUrl;
    ffmpegArgs << "-i" << audioUrl;
    ffmpegArgs << "-c:v" << "copy";
    ffmpegArgs << "-c:a" << "aac";
    ffmpegArgs << "-f" << "mp4";
    ffmpegArgs << "-";  // 输出到标准输出

    m_ffmpegProcess->start("ffmpeg", ffmpegArgs);

    // 读取 FFmpeg 输出
    connect(m_ffmpegProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_ffmpegProcess->readAllStandardOutput();
        emit sigDataReady(data);
    });

    connect(m_ffmpegProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString err = m_ffmpegProcess->readAllStandardError();
        qDebug() << "FFmpeg:" << err;
    });

    connect(m_ffmpegProcess, &QProcess::finished, this, [this](int code) {
        if (code != 0) {
            emit sigError("FFmpeg 合并失败");
        }
    });

    m_ffmpegProcess->start();
}

void NetworkPlayer::onYtError(QProcess::ProcessError error) {
    emit sigError("yt-dlp 进程错误");
}