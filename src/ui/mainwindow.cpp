#include "src/stdafx.h"
#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    :QMainWindow(parent)
{
    // 窗口基础设置
    this->resize(1280,720);
    this->setWindowTitle("jj视频播放器");
    this->setStyleSheet("QMainWindow {background: #2b2b2b;}");

    // 1. 创建中心控件 + 主布局
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0,0,0,20);
    mainLayout->setSpacing(20);

    // 2. 视频显示区域（黑色背景）
    m_videoWidget = new QWidget(this);
    m_videoWidget->setStyleSheet("background: #000000;");
    mainLayout->addWidget(m_videoWidget, 1);

    // 3. 按钮
    m_btnOpen = new QPushButton("打开", this);
    m_btnPlay = new QPushButton("播放", this);
    m_btnStop = new QPushButton("停止", this);

    QString btnStyle = "QPushButton {font-size: 14px; padding: 8px 16px; color: #fff; background: #3a3a3a; border: none; border-radius: 4px;}";
    btnStyle += "QPushButton:hover {background: #4a4a4a;} QPushButton:pressed {background: #5a5a5a;}";
    m_btnOpen->setStyleSheet(btnStyle);
    m_btnPlay->setStyleSheet(btnStyle);
    m_btnStop->setStyleSheet(btnStyle);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_btnOpen);
    btnLayout->addWidget(m_btnPlay);
    btnLayout->addWidget(m_btnStop);
    btnLayout->setSpacing(20);
    btnLayout->setContentsMargins(20,0,20,0);
    mainLayout->addLayout(btnLayout);

    // 4. 初始化播放器
    m_player = new VideoPlayer(this);
    m_sdlRender = new SDLRender(this);  // ✅ 修复：必须传 this
    m_player->setRender(m_sdlRender);

    // 5. 信号槽绑定
    connect(m_btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(m_btnPlay, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_player, &VideoPlayer::sigPlayStateChanged, this, &MainWindow::onPlayStateChanged);
    connect(m_player, &VideoPlayer::sigPlayFinished, this, &MainWindow::onPlayFinished);

    // ✅ 渲染跨线程安全
    connect(m_player, &VideoPlayer::sigRenderFrame,
            this, [this](const FrameData& frame) {
                m_sdlRender->render(frame);
            });

    // 6. 状态栏
    statusBar()->showMessage("就绪");
    statusBar()->setStyleSheet("QStatusBar {color: #fff; background: #3a3a3a;}");
}

MainWindow::~MainWindow()
{
    m_player->stop();
    delete m_sdlRender;
    delete m_player;
}

void MainWindow::onOpenFileClicked()
{
    m_currentFilePath = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv);;所有文件 (*.*)"
        );

    if(!m_currentFilePath.isEmpty()){
        if(m_player->openFile(m_currentFilePath)){
            statusBar()->showMessage("已打开：" + m_currentFilePath);
            m_btnPlay->setText("播放");
        } else {
            statusBar()->showMessage("打开文件失败！");
        }
    }
}

void MainWindow::onPlayClicked()
{
    if(m_currentFilePath.isEmpty()){
        statusBar()->showMessage("请先打开视频");
        return;
    }

    if(m_player->isPlaying()){
        m_player->pause();
        m_btnPlay->setText("播放");
        statusBar()->showMessage("已暂停");
    } else {
        m_player->play();
        m_btnPlay->setText("暂停");
        statusBar()->showMessage("正在播放");
    }
}

void MainWindow::onStopClicked()
{
    m_player->stop();
    m_btnPlay->setText("播放");
    statusBar()->showMessage("已停止");
}

void MainWindow::onPlayStateChanged(bool isPlaying)
{
    if(isPlaying){
        m_btnPlay->setText("暂停");
    }else{
        m_btnPlay->setText("播放");
    }
}

void MainWindow::onPlayFinished()
{
    m_btnPlay->setText("播放");
    statusBar()->showMessage("播放完成");
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // ✅ 最终正确 SDL 初始化
    m_sdlRender->init((void*)m_videoWidget->winId(),
                      m_videoWidget->width(),
                      m_videoWidget->height());
}