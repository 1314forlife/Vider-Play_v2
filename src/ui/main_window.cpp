#include "main_window.h"
#include "src/ui/video_widget.h"
#include "src/engine/play_engine.h"
#include "src/common/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include "src/ui/progress_bar.h"
#include "network_dialog.h"
#include <QProcess>
#include <QMenuBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupConnections();

    // 创建播放引擎
    m_engine = &PlayEngine::instance();

    // 错误：参数类型不匹配
    connect(m_engine, &PlayEngine::sigStateChanged,
            this, &MainWindow::onEngineStateChanged);
    connect(m_engine, &PlayEngine::sigError,
            this, &MainWindow::onEngineError);
    connect(m_engine, &PlayEngine::sigProgressChanged,
            this, &MainWindow::onProgressChanged);
    connect(m_progressBar, &ProgressBar::sliderPressed,
            this, &MainWindow::onSliderPressed);
    connect(m_progressBar, &ProgressBar::sliderReleased,
            this, &MainWindow::onSliderReleased);
    connect(m_videoWidget, &VideoWidget::fullscreenChanged,
            this, &MainWindow::onFullscreenChanged);
    resize(900, 600);
    setWindowTitle("视频播放器");

    LOG_INFO("MainWindow", "主窗口初始化完成");
}

MainWindow::~MainWindow() {
    if (m_engine) {
        m_engine->stop();
    }
}

void MainWindow::setupUI() {
    // 中央窗口
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 🔥 添加菜单栏
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* fileMenu = menuBar->addMenu("文件");
    QAction* networkAction = fileMenu->addAction("网络播放");
    connect(networkAction, &QAction::triggered, this, &MainWindow::onNetworkPlay);


    // 添加打开文件到菜单
    QAction* openAction = fileMenu->addAction("打开文件");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    mainLayout->addWidget(menuBar);

    // 视频显示区域
    m_videoWidget = new VideoWidget(this);
    m_videoWidget->setObjectName("videoWidget");  // 🔥 添加
    mainLayout->addWidget(m_videoWidget, 1);

    // 进度条组件
    m_progressBar = new ProgressBar(this);
    mainLayout->addWidget(m_progressBar);

    // 控制栏
    QWidget* controlBar = new QWidget(this);
    controlBar->setObjectName("controlBar");  // 🔥 添加
    controlBar->setFixedHeight(50);
    // 🔥 删除内联样式，改用 QSS
    // controlBar->setStyleSheet("background: #2b2b2b;");

    QHBoxLayout* controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(10, 5, 10, 5);
    controlLayout->setSpacing(10);

    // 🔥 按钮样式改为只设置 objectName，不写内联样式
    m_openBtn = new QPushButton("打开文件", this);
    m_openBtn->setObjectName("controlBtn");  // 🔥 添加
    controlLayout->addWidget(m_openBtn);

    m_playPauseBtn = new QPushButton("播放", this);
    m_playPauseBtn->setEnabled(false);
    m_playPauseBtn->setObjectName("controlBtn");  // 🔥 添加
    controlLayout->addWidget(m_playPauseBtn);

    m_stopBtn = new QPushButton("停止", this);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setObjectName("controlBtn");  // 🔥 添加
    controlLayout->addWidget(m_stopBtn);

    controlLayout->addStretch();

    // ========== 音量控件（添加到 controlLayout 里）==========
    QLabel* volumeLabel = new QLabel("音量", this);
    controlLayout->addWidget(volumeLabel);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(80);
    controlLayout->addWidget(m_volumeSlider);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    // ========================================================

    mainLayout->addWidget(controlBar);  // 只添加一次，不要重复
}

void MainWindow::setupConnections() {
    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    // 🔥 连接进度条组件的 seek 信号
    connect(m_progressBar, &ProgressBar::sigSeekRequested,
            this, &MainWindow::onSeekRequested);
}

void MainWindow::onOpenFile() {
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "选择视频文件", "",
                                                    "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv);;所有文件 (*.*)");

    if (filePath.isEmpty()) return;

    LOG_INFO("MainWindow", "打开文件: " + filePath);

    // 停止当前播放
    m_engine->stop();

    // 设置渲染窗口
    void* winId = m_videoWidget->getWindowId();
    m_engine->setRenderWindow(winId);

    // 打开文件
    if (m_engine->openFile(filePath)) {
        // 调整窗口大小
        int videoWidth = m_engine->width();
        int videoHeight = m_engine->height();
        resize(videoWidth + 16, videoHeight + 80);  // 加上边框和标题栏

        // 🔥 设置进度条总时长
        m_progressBar->setDuration(m_engine->duration());
        m_progressBar->setEnabled(true);

        // 开始播放
        m_engine->play();

        m_playPauseBtn->setEnabled(true);
        m_stopBtn->setEnabled(true);
        m_playPauseBtn->setText("暂停");

        LOG_INFO("MainWindow", "播放开始");
    } else {
        QMessageBox::warning(this, "错误", "无法打开视频文件");
    }
}

void MainWindow::onPlayPause() {
    if (!m_engine) return;

    if (m_engine->isPlaying()) {
        m_engine->pause();
        m_playPauseBtn->setText("播放");
        LOG_INFO("MainWindow", "暂停");
    } else if (m_engine->isPaused()) {
        m_engine->play();
        m_playPauseBtn->setText("暂停");
        LOG_INFO("MainWindow", "恢复播放");
    }
}

void MainWindow::onStop() {
    if (m_engine) {
        m_engine->stop();
        m_playPauseBtn->setText("播放");
        m_playPauseBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
        LOG_INFO("MainWindow", "停止");
    }
}

void MainWindow::onProgressChanged(int64_t current, int64_t total)
{
    LOG_DEBUG("MainWindow", QString("onProgressChanged: current=%1, total=%2")
                  .arg(current).arg(total));

    if (m_isSeeking) return;
    m_progressBar->setCurrent(current);
}

void MainWindow::onSeekRequested(int64_t position)
{
    if (m_engine) {
        LOG_INFO("MainWindow", QString("Seek 到: %1 ms").arg(position / 1000));
        m_engine->seek(position);
    }
}

void MainWindow::onEngineStateChanged(PlaybackState state) {
    // 更新按钮状态
    bool isPlaying = (state == PlaybackState::Playing);  // Playing
    m_playPauseBtn->setText(isPlaying ? "暂停" : "播放");
}

void MainWindow::onEngineError(const QString& error) {
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
    NetworkDialog dialog(this);

    connect(&dialog, &NetworkDialog::urlReady, this, [this](const QString& url) {
        m_engine->stop();
        void* winId = m_videoWidget->getWindowId();
        m_engine->setRenderWindow(winId);

        if (m_engine->openFile(url)) {
            // 延迟1秒让网络流缓冲
            QTimer::singleShot(1000, this, [this]() {
                m_progressBar->setDuration(m_engine->duration());
                m_engine->play();
                m_playPauseBtn->setText("暂停");
                m_playPauseBtn->setEnabled(true);
                m_stopBtn->setEnabled(true);
            });
        } else {
            QMessageBox::warning(this, "错误", "无法播放该链接");
        }
    });

    dialog.exec();
}

void MainWindow::onFullscreenChanged(bool fullscreen)
{
    if (fullscreen) {
        showFullScreen();
    } else {
        showNormal();
    }
}

void MainWindow::onVolumeChanged(int volume)
{
    if (m_engine) {
        m_engine->setVolume(volume);
    }
}


void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    // 通知渲染器窗口大小改变
    if (m_engine && m_videoWidget) {
        m_engine->setRenderWindowSize(m_videoWidget->width(), m_videoWidget->height());
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_engine) {
        m_engine->stop();
    }
    event->accept();
}