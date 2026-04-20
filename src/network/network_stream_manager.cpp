#include "network_stream_manager.h"
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QTemporaryFile>

NetworkStreamManager::NetworkStreamManager(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_port(12345)
    , m_isReady(false)
{
    setupProcess();
}

NetworkStreamManager::~NetworkStreamManager()
{
    stopStream();
}

void NetworkStreamManager::setupProcess()
{
    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
        if (m_process->state() == QProcess::Running) {
            m_process->terminate();
            m_process->waitForFinished(2000);
        }
        delete m_process;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
}

void NetworkStreamManager::startStream(const QString &bilibiliUrl)
{
    if (bilibiliUrl.isEmpty()) {
        emit streamError("URL is empty");
        return;
    }

    if (isRunning()) {
        stopStream();
    }

    m_isReady = false;
    m_localUrl.clear();
    setupProcess();

    // ====================== 【彻底修复崩溃】创建 BAT 文件 ======================
    QString batPath = QDir::currentPath() + "/run_stream.bat";
    QFile batFile(batPath);

    if (!batFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit streamError("无法创建批处理文件");
        return;
    }

    QString command = QString(
                          "yt-dlp --cookies \"cookies.txt\" --referer \"https://www.bilibili.com\" "
                          "-f \"bestvideo[height<=720][ext=mp4]+bestaudio[ext=m4a]\" "
                          "--merge-output-format mkv -o - \"%1\" "
                          "| ffmpeg -i pipe:0 -c:v libx264 -preset ultrafast -c:a aac -f mpegts tcp://127.0.0.1:%2?listen"
                          ).arg(bilibiliUrl).arg(m_port);

    QTextStream out(&batFile);
    out << "@echo off\n";
    out << command;
    batFile.close();

    emit logMessage("Command: " + command);

    // ====================== 运行 .bat 文件，永不崩溃 ======================
    m_process->start(batPath, QStringList());

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        emit logMessage(output);

        if (!m_isReady && (output.contains("tcp://") || output.contains("Listening") || output.contains("Press [q] to stop"))) {
            QTimer::singleShot(800, this, [this]() {
                m_isReady = true;
                m_localUrl = QString("tcp://127.0.0.1:%1").arg(m_port);
                emit streamReady(m_localUrl);
            });
        }
    });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        emit streamError("Process error: " + QString::number(error));
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                if (exitCode != 0 && !m_isReady) {
                    emit streamError("Process exit: " + QString::number(exitCode));
                }
                m_isReady = false;
            });

    QTimer::singleShot(30000, this, [this]() {
        if (m_process && m_process->state() == QProcess::Running && !m_isReady) {
            emit streamError("Timeout");
            stopStream();
        }
    });
}

void NetworkStreamManager::stopStream()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }
    m_isReady = false;
    m_localUrl.clear();
}

bool NetworkStreamManager::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}