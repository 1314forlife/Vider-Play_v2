#include "network_stream_manager.h"
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QThread>
#include <QCoreApplication>

NetworkStreamManager::NetworkStreamManager(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_port(1935)
    , m_isReady(false)
    , m_isStopping(false)
    , m_restartTimer(nullptr)
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

    // ✅ 去重：同一个 URL 已经在推流
    if (m_currentUrl == bilibiliUrl && isRunning()) {
        emit logMessage("Same URL already pushing, skip");
        return;
    }

    // ✅ 如果需要切换 URL，先安全停止
    if (isRunning()) {
        m_currentUrl.clear();
        stopStream();
        m_isStopping = true;

        if (!m_restartTimer) {
            m_restartTimer = new QTimer(this);
            m_restartTimer->setSingleShot(true);
            connect(m_restartTimer, &QTimer::timeout, this, &NetworkStreamManager::onDelayedStart);
        }

        m_restartTimer->start(2000);
        m_pendingUrl = bilibiliUrl;
        return;
    }

    m_currentUrl = bilibiliUrl;
    m_isReady = false;
    m_localUrl.clear();
    setupProcess();

    QString appDir = QCoreApplication::applicationDirPath();
    QString batPath = appDir + "/run_stream.bat";

    // ✅ 检查 bat 是否存在
    if (!QFile::exists(batPath)) {
        emit streamError("run_stream.bat not found in: " + appDir);
        return;
    }

    // ✅ 直接传参启动，不再修改 bat 内容
    m_process->start("cmd.exe", QStringList() << "/c" << batPath << bilibiliUrl);

    // ✅ 只输出日志，不依赖字符串触发
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        emit logMessage(QString::fromUtf8(m_process->readAllStandardError()));
    });

    // ✅ 固定 5 秒后认为推流就绪
    QTimer::singleShot(5000, this, [this]() {
        if (!m_isReady) {
            m_isReady = true;
            m_localUrl = "rtmp://127.0.0.1/live/stream";
            emit streamReady(m_localUrl);
            emit logMessage("推流已就绪（固定延迟）");
        }
    });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        emit streamError("Process error: " + QString::number(error));
    });

    connect(m_process, &QProcess::finished, this, &NetworkStreamManager::onProcessFinished);

    QTimer::singleShot(30000, this, [this]() {
        if (m_process && m_process->state() == QProcess::Running && !m_isReady) {
            emit streamError("30秒超时，未检测到推流");
            stopStream();
        }
    });
}

void NetworkStreamManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (exitCode != 0 && !m_isReady) {
        emit streamError("Process exit: " + QString::number(exitCode));
    }
    m_isReady = false;
    m_currentUrl.clear();

    if (m_isStopping) {
        m_isStopping = false;
    }
}

void NetworkStreamManager::onDelayedStart()
{
    if (!m_pendingUrl.isEmpty()) {
        startStream(m_pendingUrl);
        m_pendingUrl.clear();
    }
}

void NetworkStreamManager::stopStream()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
    m_isReady = false;
    m_localUrl.clear();
    m_currentUrl.clear();
}

bool NetworkStreamManager::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}