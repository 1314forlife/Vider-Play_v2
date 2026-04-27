#include "network_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>

NetworkDialog::NetworkDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("网络播放");
    setFixedSize(500, 180);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("请输入B站视频链接:"));

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText("https://www.bilibili.com/video/BV1xx...");
    layout->addWidget(m_urlEdit);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    layout->addWidget(m_progressBar);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_playBtn = new QPushButton("播放", this);
    m_cancelBtn = new QPushButton("取消", this);
    btnLayout->addWidget(m_playBtn);
    btnLayout->addWidget(m_cancelBtn);
    layout->addLayout(btnLayout);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: gray;");
    layout->addWidget(m_statusLabel);

    connect(m_playBtn, &QPushButton::clicked, this, &NetworkDialog::onPlay);
    connect(m_cancelBtn, &QPushButton::clicked, this, &NetworkDialog::onCancel);

    m_streamMgr = new NetworkStreamManager(this);
}

NetworkDialog::~NetworkDialog()
{
    // ✅ 确保推流进程停止，避免残留
    if (m_streamMgr) {
        m_streamMgr->stopStream();
    }
}

void NetworkDialog::onPlay()
{
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入视频链接");
        return;
    }

    if (!url.contains("bilibili.com")) {
        QMessageBox::warning(this, "错误", "目前仅支持B站视频链接");
        return;
    }

    m_playBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    m_statusLabel->setText("正在解析视频并启动推流...");
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    // ✅ 断开旧的连接，避免重复
    disconnect(m_streamMgr, nullptr, this, nullptr);

    connect(m_streamMgr, &NetworkStreamManager::logMessage, this, &NetworkDialog::onLogMessage);
    connect(m_streamMgr, &NetworkStreamManager::streamError, this, &NetworkDialog::onError);
    connect(m_streamMgr, &NetworkStreamManager::streamReady, this, &NetworkDialog::onPlayReady);

    m_streamMgr->startStream(url);
}

void NetworkDialog::onCancel()
{
    if (m_streamMgr) {
        m_streamMgr->stopStream();
    }
    reject();
}

void NetworkDialog::onLogMessage(const QString& msg)
{
    if (msg.contains("yt-dlp")) {
        m_progressBar->setValue(20);
    } else if (msg.contains("ffmpeg") || msg.contains("frame=")) {
        m_progressBar->setValue(60);
    }
    m_statusLabel->setText(msg);
}

void NetworkDialog::onError(const QString& error)
{
    m_playBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);
    m_progressBar->setVisible(false);
    m_statusLabel->setText("播放失败");
    QMessageBox::warning(this, "错误", error);
}

void NetworkDialog::onPlayReady(const QString& localUrl)
{
    m_progressBar->setValue(100);
    m_statusLabel->setText("播放就绪");

    // ✅ 确保在关闭前发射信号
    emit urlReady(localUrl);

    // ✅ 延迟关闭，避免信号冲突
    QTimer::singleShot(200, this, &QDialog::accept);
}