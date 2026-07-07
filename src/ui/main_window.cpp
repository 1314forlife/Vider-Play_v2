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
#include "src/network/LicenseClient.h"
#include "src/ui/LoginDialog.h"
#include "network_dialog.h"
#include "toolbox/videotoolbox.h"

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

    // ========== 添加许可证客户端信号连接 ==========
    connect(LicenseClient::instance(), &LicenseClient::licenseReady,
            this, &MainWindow::onLicenseReady);
    connect(LicenseClient::instance(), &LicenseClient::licenseFailed,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "播放失败", "获取许可证失败: " + error);
                m_isLicenseRequired = false;
            });
    // ===========================================

    connect(m_engine, &PlayEngine::qualityStreamsReady,
            this, &MainWindow::onQualityStreamsReady);

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
    m_centralStack->addWidget(createToolboxPage()); // 索引3: 设置页面
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
    connect(m_navigation, &NavigationWidget::toolboxClicked, this, &MainWindow::switchToToolbox);
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

    // ========== 添加清晰度下拉框 ==========
    m_qualityCombo = new QComboBox(page);
    m_qualityCombo->setFixedWidth(100);
    m_qualityCombo->setVisible(false);  // 默认隐藏，有多码流时再显示
    controlLayout->addWidget(m_qualityCombo);
    // ===================================

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

QWidget* MainWindow::createToolboxPage()
{
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 使用真正的 VideoToolBox 类
    VideoToolBox* toolBox = new VideoToolBox(page);
    toolBox->setEmbedMode(true);
    layout->addWidget(toolBox);

    // 连接信号：处理完成后自动加载到播放器
    connect(toolBox, &VideoToolBox::videoProcessed,
            this, [this](const QString& outputPath) {
                playLocalFile(outputPath);
                switchToPlayer();
            });

    return page;
}

void MainWindow::checkLoginAndPlay(const QString& videoId)
{
    if (m_currentToken.isEmpty()) {
        // 未登录，弹出登录对话框
        LoginDialog* dialog = new LoginDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &LoginDialog::loginSuccess, this, [this, videoId](const QString& token) {
            m_currentToken = token;
            // 登录成功后请求许可证
            if (LicenseClient::instance()) {
                LicenseClient::instance()->setToken(token);
                LicenseClient::instance()->requestLicense(videoId);
            }
        });
        dialog->show();
    } else {
        // 已登录，直接请求许可证
        if (LicenseClient::instance()) {
            LicenseClient::instance()->requestLicense(videoId);
        }
    }
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

void MainWindow::switchToToolbox()
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

void MainWindow::onLoginSuccess(const QString& token)
{
    m_currentToken = token;
    LOG_INFO("MainWindow", "登录成功，token已保存");
    // 可以显示一个提示
    // QMessageBox::information(this, "提示", "登录成功");
}

void MainWindow::onPlayVideo(const QString& videoId)
{
    m_currentVideoId = videoId;
    checkLoginAndPlay(videoId);
}

void MainWindow::onQualityStreamsReady(const QVector<StreamInfo>& streams, int defaultIndex) {
    // 更新清晰度下拉框
    m_qualityCombo->clear();
    for (int i = 0; i < streams.size(); i++) {
        const auto& stream = streams[i];
        QString text = QString("%1 (%2 kbps)").arg(stream.resolution).arg(stream.bandwidth / 1000);
        m_qualityCombo->addItem(text, i);
    }
    m_qualityCombo->setCurrentIndex(defaultIndex);
    m_qualityCombo->setVisible(true);

    // 连接信号（只连接一次）
    static bool connected = false;
    if (!connected) {
        connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onQualityChanged);
        connected = true;
        qDebug() << "清晰度下拉框信号已连接";
    }
}
// ========== 播放器功能 ==========

void MainWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "选择视频文件", "",
                                                    "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv);;所有文件 (*.*)");
    if (filePath.isEmpty()) return;

    // 判断是否需要许可证（如果是加密HLS流）
    if (filePath.startsWith("http") && filePath.contains("m3u8")) {
        // 提取视频ID，例如从文件名或路径
        m_currentVideoId = "demo_001";  // 实际应该从URL或用户选择获取
        m_isLicenseRequired = true;

        // 检查是否已登录
        if (LicenseClient::instance()->isLoggedIn()) {
            // 已登录，直接请求许可证
            LicenseClient::instance()->requestLicense(m_currentVideoId);
        } else {
            // 未登录，弹出登录框
            LoginDialog* dialog = new LoginDialog(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            connect(dialog, &LoginDialog::loginSuccess, this, [this](const QString& token) {
                LicenseClient::instance()->setToken(token);
                LicenseClient::instance()->requestLicense(m_currentVideoId);
            });
            dialog->show();
        }
        return;
    }

    // 普通文件直接播放
    m_isLicenseRequired = false;
    playLocalFile(filePath);
}

void MainWindow::playLocalFile(const QString& filePath)
{
    m_engine->stop();
    void* winId = m_videoWidget->getWindowId();
    m_engine->setRenderWindow(winId);

    if (m_engine->openFile(filePath)) {
        // ✅ 不调整窗口大小，保持当前窗口尺寸
        // 视频画面会自动适应 VideoWidget 的大小
        // int videoWidth = m_engine->width();
        // int videoHeight = m_engine->height();
        // resize(videoWidth + 16, videoHeight + 80);  // ← 注释掉这行

        m_progressBar->setDuration(m_engine->duration());
        m_progressBar->setEnabled(true);

        m_engine->play();
        m_playPauseBtn->setEnabled(true);
        m_stopBtn->setEnabled(true);
        m_playPauseBtn->setText("暂停");
        LOG_INFO("MainWindow", "本地文件播放开始");

        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.3);
        }
    } else {
        QMessageBox::warning(this, "错误", "无法打开视频文件");
    }
}

void MainWindow::updateQualityCombo() {
    auto streams = m_engine->getStreams();
    if (streams.isEmpty()) {
        if (m_qualityCombo) m_qualityCombo->setVisible(false);
        return;
    }

    if (!m_qualityCombo) {
        // 找到控制栏的位置添加
        // 假设你有个 controlLayout
        m_qualityCombo = new QComboBox(this);
        m_qualityCombo->setFixedWidth(100);
        // controlLayout->addWidget(m_qualityCombo);
        connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onQualityChanged);
    }

    m_qualityCombo->clear();
    for (const auto& stream : streams) {
        QString text = QString("%1 (%2 kbps)").arg(stream.resolution).arg(stream.bandwidth / 1000);
        m_qualityCombo->addItem(text, stream.index);
    }
    m_qualityCombo->setVisible(true);

    // 设置当前选中的清晰度
    int currentIndex = m_engine->currentStreamIndex();
    if (currentIndex >= 0) {
        m_qualityCombo->setCurrentIndex(currentIndex);
    }
}

void MainWindow::onQualityChanged(int index) {
    if (index < 0 || !m_engine) return;

    int streamIndex = m_qualityCombo->itemData(index).toInt();
    if (streamIndex != m_engine->currentStreamIndex()) {
        m_engine->switchToStream(streamIndex);
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
    // 直接显示网络对话框，跳过登录
    NetworkDialog* dialog = new NetworkDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dialog, &NetworkDialog::urlReady, this, [this, dialog](const QString& url) {
        dialog->accept();
        dialog->deleteLater();

        m_currentStreamUrl = url;

        if (url.contains("bilibili.com")) {
            if (!m_networkManager) {
                m_networkManager = new NetworkStreamManager(this);
                connect(m_networkManager, &NetworkStreamManager::streamReady,
                        this, &MainWindow::onStreamReady);
                connect(m_networkManager, &NetworkStreamManager::streamError,
                        this, &MainWindow::onStreamError);
            }
            m_networkManager->startStream(url);
        } else if (url.contains(".m3u8")) {
            // HLS 点播：直接播放，不需要许可证和登录
            LOG_INFO("MainWindow", "直接播放 HLS: " + url);
            playDirect(url);
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
        // ✅ 延迟恢复几何位置，避免冲突
        QTimer::singleShot(50, this, [this]() {
            if (!m_normalGeometry.isEmpty()) {
                setGeometry(m_normalGeometry);
            }
        });
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

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    static bool isResizing = false;
    if (isResizing) return;
    isResizing = true;

    // ✅ 只有窗口真正改变时才更新渲染器
    if (m_engine && m_videoWidget && event->size() != event->oldSize()) {
        m_engine->setRenderWindowSize(m_videoWidget->width(), m_videoWidget->height());
    }

    isResizing = false;
}

void MainWindow::onLicenseReady(const QString& licenseKey)
{
    if (!m_isLicenseRequired) return;

    LOG_INFO("MainWindow", "收到许可证密钥: " + licenseKey);

    // TODO: 根据你的播放引擎，设置解密密钥
    // 方式1：如果 PlayEngine 支持设置解密密钥
    // m_engine->setDecryptionKey(licenseKey);

    // 方式2：如果是 HLS 流，可能需要修改 m3u8 的 URL
    // 例如：添加解密参数
    QString streamUrl = m_currentStreamUrl; // 你需要保存当前URL
    if (streamUrl.contains("m3u8") && !licenseKey.isEmpty()) {
        // 某些 HLS 播放器支持通过 URL 参数传递密钥
        // streamUrl += "?decryption_key=" + licenseKey;
    }

    // 播放加密流
    void* winId = m_videoWidget->getWindowId();
    m_engine->setRenderWindow(winId);

    if (m_engine->openFile(streamUrl)) {
        m_progressBar->setDuration(m_engine->duration());
        m_progressBar->setEnabled(true);
        m_engine->play();
        m_playPauseBtn->setText("暂停");
        m_playPauseBtn->setEnabled(true);
        m_stopBtn->setEnabled(true);

        if (m_furinaLottie) {
            m_furinaLottie->setOpacity(0.3);
        }
    } else {
        QMessageBox::warning(this, "错误", "无法播放加密视频");
    }

    m_isLicenseRequired = false;
}