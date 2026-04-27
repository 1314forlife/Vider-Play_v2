#include "main_window.h"
#include "src/ui/video_widget.h"
#include "src/ui/title_bar.h"
#include "src/ui/navigation_widget.h"
#include "src/ui/download_widget.h"
#include "src/engine/play_engine.h"
#include "src/common/logger.h"
#include "src/ui/progress_bar.h"
#include "src/theme/theme_manager.h"
#include "src/ui/furina_lottie.h"
#include "network_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QLabel>
#include <QSlider>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setupUI();
    setupConnections();

    // 创建播放引擎
    m_engine = &PlayEngine::instance();

    connect(m_engine, &PlayEngine::sigStateChanged,
            this, &MainWindow::onEngineStateChanged);
    connect(m_engine, &PlayEngine::sigError,
            this, &MainWindow::onEngineError);
    connect(m_engine, &PlayEngine::sigProgressChanged,
            this, &MainWindow::onProgressChanged);

    resize(900, 600);
    setWindowTitle("芙宁娜·水之颂");

    LOG_INFO("MainWindow", "主窗口初始化完成");
}

MainWindow::~MainWindow()
{
    if (m_engine) {
        m_engine->stop();
    }
}

void MainWindow::setupUI()
{
    // 设置无边框窗口
    setWindowFlags(Qt::FramelessWindowHint);

    // 创建中央控件
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 标题栏
    m_titleBar = new TitleBar(this);
    m_titleBar->setTitle("芙宁娜·水之颂 播放器");
    mainLayout->addWidget(m_titleBar);

    // 2. 内容区域（水平布局：导航 + 堆叠页面）
    QWidget* contentWidget = new QWidget(this);
    QHBoxLayout* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 左侧导航
    m_navigation = new NavigationWidget(this);
    contentLayout->addWidget(m_navigation);

    // 右侧堆叠页面
    m_centralStack = new QStackedWidget(this);
    m_centralStack->addWidget(createPlayerPage());   // 索引0: 播放页面
    m_centralStack->addWidget(createDownloadPage()); // 索引1: 下载页面
    m_centralStack->addWidget(createThemePage());    // 索引2: 主题页面
    m_centralStack->addWidget(createSettingsPage()); // 索引3: 设置页面
    contentLayout->addWidget(m_centralStack, 1);

    mainLayout->addWidget(contentWidget, 1);
}

void MainWindow::setupConnections()
{
    // 标题栏信号
    connect(m_titleBar, &TitleBar::minimizeClicked, this, &QMainWindow::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, &MainWindow::toggleMaximize);
    connect(m_titleBar, &TitleBar::closeClicked, this, &QMainWindow::close);

    // 导航栏信号
    connect(m_navigation, &NavigationWidget::playerClicked, this, &MainWindow::switchToPlayer);
    connect(m_navigation, &NavigationWidget::downloadClicked, this, &MainWindow::switchToDownload);
    connect(m_navigation, &NavigationWidget::themeClicked, this, &MainWindow::switchToTheme);
    connect(m_navigation, &NavigationWidget::settingsClicked, this, &MainWindow::switchToSettings);
}

QWidget* MainWindow::createPlayerPage()
{
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 视频显示区域
    m_videoWidget = new VideoWidget(page);
    m_videoWidget->setObjectName("videoWidget");
    layout->addWidget(m_videoWidget, 1);

    // 进度条
    m_progressBar = new ProgressBar(page);
    layout->addWidget(m_progressBar);

    // 控制栏
    QWidget* controlBar = new QWidget(page);
    controlBar->setObjectName("controlBar");
    controlBar->setFixedHeight(50);

    QHBoxLayout* controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(10, 5, 10, 5);
    controlLayout->setSpacing(10);

    // 打开文件按钮
    m_openBtn = new QPushButton("打开文件", page);
    m_openBtn->setObjectName("controlBtn");
    controlLayout->addWidget(m_openBtn);
    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);

    // 播放/暂停按钮
    m_playPauseBtn = new QPushButton("播放", page);
    m_playPauseBtn->setEnabled(false);
    m_playPauseBtn->setObjectName("controlBtn");
    controlLayout->addWidget(m_playPauseBtn);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);

    // 停止按钮
    m_stopBtn = new QPushButton("停止", page);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setObjectName("controlBtn");
    controlLayout->addWidget(m_stopBtn);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);

    controlLayout->addStretch();

    // 网络播放按钮
    QPushButton* networkBtn = new QPushButton("网络播放", page);
    networkBtn->setObjectName("controlBtn");
    controlLayout->addWidget(networkBtn);
    connect(networkBtn, &QPushButton::clicked, this, &MainWindow::onNetworkPlay);

    // 音量控制
    QLabel* volumeLabel = new QLabel("🔊", page);
    controlLayout->addWidget(volumeLabel);

    m_volumeSlider = new QSlider(Qt::Horizontal, page);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(80);
    controlLayout->addWidget(m_volumeSlider);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);

    // 全屏按钮
    QPushButton* fullscreenBtn = new QPushButton("⛶", page);
    fullscreenBtn->setObjectName("controlBtn");
    fullscreenBtn->setFixedWidth(40);
    controlLayout->addWidget(fullscreenBtn);
    connect(fullscreenBtn, &QPushButton::clicked, this, [this]() {
        if (m_videoWidget) {
            m_videoWidget->setFullscreen(!m_videoWidget->isFullscreen());
        }
    });

    layout->addWidget(controlBar);

    // 连接进度条信号
    connect(m_progressBar, &ProgressBar::sliderPressed, this, &MainWindow::onSliderPressed);
    connect(m_progressBar, &ProgressBar::sliderReleased, this, &MainWindow::onSliderReleased);
    connect(m_progressBar, &ProgressBar::sigSeekRequested, this, &MainWindow::onSeekRequested);

    // 连接全屏信号
    connect(m_videoWidget, &VideoWidget::fullscreenChanged, this, &MainWindow::onFullscreenChanged);

    // 创建芙宁娜动画悬浮层
    m_furinaLottie = new FurinaLottie(m_videoWidget);
    m_furinaLottie->resize(m_videoWidget->size());
    m_furinaLottie->showGoddess();

    // 监听视频控件大小变化
    connect(m_videoWidget, &VideoWidget::resized, [this]() {
        if (m_furinaLottie) {
            m_furinaLottie->setGeometry(m_videoWidget->geometry());
        }
    });

    return page;
}

QWidget* MainWindow::createDownloadPage()
{
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_downloadWidget = new DownloadWidget(page);
    layout->addWidget(m_downloadWidget);

    return page;
}

QWidget* MainWindow::createThemePage()
{
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* label = new QLabel("🎨 主题切换\n\n开发中，敬请期待...", page);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #7EC8E3; font-size: 16px;");

    layout->addWidget(label);
    return page;
}

QWidget* MainWindow::createSettingsPage()
{
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* label = new QLabel("⚙️ 设置\n\n开发中，敬请期待...", page);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #7EC8E3; font-size: 16px;");

    layout->addWidget(label);
    return page;
}

// ========== 页面切换 ==========
void MainWindow::switchToPlayer()
{
    m_centralStack->setCurrentIndex(0);
    m_navigation->setActiveButton(0);
    if (m_furinaLottie) {
        m_furinaLottie->showGoddess();
    }
}

void MainWindow::switchToDownload()
{
    m_centralStack->setCurrentIndex(1);
    m_navigation->setActiveButton(1);
    if (m_furinaLottie) {
        m_furinaLottie->setOpacity(0.5);
    }
}

void MainWindow::switchToTheme()
{
    m_centralStack->setCurrentIndex(2);
    m_navigation->setActiveButton(2);
    if (m_furinaLottie) {
        m_furinaLottie->setOpacity(1.0);
    }
}

void MainWindow::switchToSettings()
{
    m_centralStack->setCurrentIndex(3);
    m_navigation->setActiveButton(3);
    if (m_furinaLottie) {
        m_furinaLottie->setOpacity(0.3);
    }
}

// ========== 窗口控制 ==========
void MainWindow::toggleMaximize()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void MainWindow::onStreamReady(const QString& streamUrl)
{
    m_engine->stop();
    void* winId = m_videoWidget->getWindowId();
    m_engine->setRenderWindow(winId);
    if (m_engine->openFile(streamUrl)) {
        m_engine->play();
        m_playPauseBtn->setText("暂停");
        m_playPauseBtn->setEnabled(true);
        m_stopBtn->setEnabled(true);
    }
}

void MainWindow::onStreamError(const QString& err)
{
    QMessageBox::warning(this, "推流失败", err);
}

void MainWindow::playDirect(const QString& url)
{
    m_engine->stop();
    void* winId = m_videoWidget->getWindowId();
    m_engine->setRenderWindow(winId);

    if (m_engine->openFile(url)) {
        m_engine->play();
        m_playPauseBtn->setText("暂停");
        m_playPauseBtn->setEnabled(true);
        m_stopBtn->setEnabled(true);
    } else {
        QMessageBox::warning(this, "错误", "无法播放该链接");
    }
}

// ========== 播放器功能 ==========

void MainWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "选择视频文件", "",
                                                    "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv);;所有文件 (*.*)");
    if (filePath.isEmpty()) return;

    LOG_INFO("MainWindow", "打开文件: " + filePath);

    m_engine->stop();
    void* winId = m_videoWidget->getWindowId();
    m_engine->setRenderWindow(winId);

    if (m_engine->openFile(filePath)) {
        int videoWidth = m_engine->width();
        int videoHeight = m_engine->height();
        resize(videoWidth + 16, videoHeight + 80);

        m_progressBar->setDuration(m_engine->duration());
        m_progressBar->setEnabled(true);

        m_engine->play();
        m_playPauseBtn->setEnabled(true);
        m_stopBtn->setEnabled(true);
        m_playPauseBtn->setText("暂停");
        LOG_INFO("MainWindow", "播放开始");

        // 播放时降低芙宁娜透明度
        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.3);
        }
    } else {
        QMessageBox::warning(this, "错误", "无法打开视频文件");
    }
}

void MainWindow::onPlayPause()
{
    if (!m_engine) return;

    if (m_engine->isPlaying()) {
        m_engine->pause();
        m_playPauseBtn->setText("播放");
        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.7);
        }
        LOG_INFO("MainWindow", "暂停");
    } else if (m_engine->isPaused()) {
        m_engine->play();
        m_playPauseBtn->setText("暂停");
        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.3);
        }
        LOG_INFO("MainWindow", "恢复播放");
    }
}

void MainWindow::onStop()
{
    if (m_engine) {
        m_engine->stop();
        m_playPauseBtn->setText("播放");
        m_playPauseBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(1.0);
        }
        LOG_INFO("MainWindow", "停止");
    }
}

void MainWindow::onProgressChanged(int64_t current, int64_t total)
{
    LOG_DEBUG("MainWindow", QString("onProgressChanged: current=%1, total=%2").arg(current).arg(total));
    if (m_isSeeking) return;
    m_progressBar->setCurrent(current);
}

void MainWindow::onSeekRequested(int64_t position)
{
    if (m_engine) {
        LOG_INFO("MainWindow", QString("Seek 到: %1 ms").arg(position / 1000));
        m_engine->seek(position);
        if (m_furinaLottie) {
            m_furinaLottie->triggerSplash();
        }
    }
}

void MainWindow::onEngineStateChanged(PlaybackState state)
{
    bool isPlaying = (state == PlaybackState::Playing);
    m_playPauseBtn->setText(isPlaying ? "暂停" : "播放");
}

void MainWindow::onEngineError(const QString& error)
{
    LOG_ERROR("MainWindow", error);
    QMessageBox::critical(this, "播放错误", error);
}

void MainWindow::onSliderPressed()
{
    m_isSeeking = true;
}

void MainWindow::onSliderReleased()
{
    m_isSeeking = false;
}

void MainWindow::onNetworkPlay()
{
    // 不再在 lambda 里使用 dialog 对象
    NetworkDialog* dialog = new NetworkDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // ✅ 只连一个信号：拿到 URL 后就关闭对话框
    connect(dialog, &NetworkDialog::urlReady, this, [this, dialog](const QString& url) {
        dialog->accept();   // 先关闭对话框
        dialog->deleteLater();

        if (url.contains("bilibili.com")) {
            if (!m_networkManager) {
                m_networkManager = new NetworkStreamManager(this);
                connect(m_networkManager, &NetworkStreamManager::streamReady,
                        this, &MainWindow::onStreamReady);
                connect(m_networkManager, &NetworkStreamManager::streamError,
                        this, &MainWindow::onStreamError);
            }
            m_networkManager->startStream(url);
        } else {
            playDirect(url);
        }
    });

    dialog->show();
}

void MainWindow::onFullscreenChanged(bool fullscreen)
{
    if (fullscreen) {
        if (!isFullScreen()) {
            m_normalGeometry = geometry();
        }
        m_titleBar->hide();
        m_navigation->hide();
        showFullScreen();
        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.15);
        }
    } else {
        m_titleBar->show();
        m_navigation->show();
        showNormal();
        setGeometry(m_normalGeometry);
        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.3);
        }
    }
}

void MainWindow::onVolumeChanged(int volume)
{
    if (m_engine) {
        m_engine->setVolume(volume);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (m_engine && m_videoWidget) {
        m_engine->setRenderWindowSize(m_videoWidget->width(), m_videoWidget->height());
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_networkManager) {
        m_networkManager->stopStream();
    }

    if (isFullScreen()) {
        onFullscreenChanged(false);
    }
    event->accept();

    if (m_engine) {
        QTimer::singleShot(0, this, [this]() {
            m_engine->stop();
        });
    }
}