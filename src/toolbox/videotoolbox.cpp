// src/toolbox/videotoolbox.cpp
#include "videotoolbox.h"
#include "transcodewidget.h"
#include "audioextractwidget.h"
#include "cropwidget.h"
#include "screenshotwidget.h"
#include "formatconvertwidget.h"
#include "compresswidget.h"
#include "watermarkremover.h"

#include <QSplitter>
#include <QHeaderView>
#include <QStyle>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>

VideoToolBox::VideoToolBox(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    initConnections();
}

VideoToolBox::~VideoToolBox()
{
}

void VideoToolBox::initUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 左侧导航面板 ==========
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(160);
    leftPanel->setStyleSheet("background-color: #1A2A3A;");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 20, 10, 20);
    leftLayout->setSpacing(5);

    QLabel *titleLabel = new QLabel("🛠️ 工具箱", leftPanel);
    titleLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold; padding: 10px 0 20px 0;");
    leftLayout->addWidget(titleLabel);

    m_toolList = new QListWidget(leftPanel);
    m_toolList->setStyleSheet(
        "QListWidget { background: transparent; border: none; color: #B0C4DE; font-size: 14px; outline: none; }"
        "QListWidget::item { padding: 12px 16px; border-radius: 6px; margin: 2px 0; }"
        "QListWidget::item:hover { background: rgba(255, 255, 255, 0.08); }"
        "QListWidget::item:selected { background: #4A90D9; color: white; }"
        );

    m_toolList->addItem("🎬 视频转码");   // 索引 0
    m_toolList->addItem("🚫 去水印");     // 索引 1
    m_toolList->addItem("✂️ 视频裁剪");   // 索引 2   // 索引 3
    m_toolList->addItem("🔄 格式转换");   // 索引 4
    m_toolList->addItem("📦 视频压缩");   // 索引 5
    m_toolList->addItem("🎵 音频提取");   // 索引 6
    m_toolList->addItem("📷 截图工具");   // 索引 7
    m_toolList->addItem("⚡ 批量处理");   // 索引 8
    leftLayout->addWidget(m_toolList);
    leftLayout->addStretch();

    QLabel *versionLabel = new QLabel("v2.0.0", leftPanel);
    versionLabel->setStyleSheet("color: #5A7A8A; font-size: 11px; padding: 10px 0;");
    leftLayout->addWidget(versionLabel);

    // ========== 右侧内容面板 ==========
    QWidget *rightPanel = new QWidget(this);
    rightPanel->setStyleSheet("background-color: #F5F7FA;");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(10);

    m_stackWidget = new QStackedWidget(rightPanel);

    // 索引0：视频转码 ✅
    TranscodeWidget *transcodeWidget = new TranscodeWidget(rightPanel);
    transcodeWidget->setStyleSheet("background-color: white; border-radius: 8px;");
    m_stackWidget->addWidget(transcodeWidget);

    // 索引1：去水印（占位）
    QScrollArea *watermarkScrollArea = new QScrollArea(rightPanel);
    watermarkScrollArea->setStyleSheet(
        "QScrollArea { background-color: white; border: none; border-radius: 8px; }"
        "QScrollBar:vertical { background: #F0F0F0; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #C0C0C0; border-radius: 4px; min-height: 30px; }"
        );
    watermarkScrollArea->setWidgetResizable(true);

    QWidget *watermarkContent = new QWidget;
    QVBoxLayout *watermarkContentLayout = new QVBoxLayout(watermarkContent);
    watermarkContentLayout->setContentsMargins(0, 0, 0, 0);
    WatermarkRemover *watermarkWidget = new WatermarkRemover(watermarkContent);
    watermarkContentLayout->addWidget(watermarkWidget);
    watermarkScrollArea->setWidget(watermarkContent);

    m_stackWidget->addWidget(watermarkScrollArea);

    // 索引2：视频裁剪 ✅
    QScrollArea *cropScrollArea = new QScrollArea(rightPanel);
    cropScrollArea->setStyleSheet(
        "QScrollArea {"
        "    background-color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "    background-color: white;"
        "}"
        "QScrollBar:vertical {"
        "    background: #F0F0F0;"
        "    width: 8px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #C0C0C0;"
        "    border-radius: 4px;"
        "    min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #A0A0A0;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        );
    cropScrollArea->setWidgetResizable(true);

    QWidget *cropContent = new QWidget;
    QVBoxLayout *cropContentLayout = new QVBoxLayout(cropContent);
    cropContentLayout->setContentsMargins(0, 0, 0, 0);
    CropWidget *cropWidget = new CropWidget(cropContent);
    cropContentLayout->addWidget(cropWidget);
    cropScrollArea->setWidget(cropContent);

    m_stackWidget->addWidget(cropScrollArea);  // 索引2


    // 索引4：格式转换（占位）
    QScrollArea *convertScrollArea = new QScrollArea(rightPanel);
    convertScrollArea->setStyleSheet(
        "QScrollArea { background-color: white; border: none; border-radius: 8px; }"
        "QScrollBar:vertical { background: #F0F0F0; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #C0C0C0; border-radius: 4px; min-height: 30px; }"
        );
    convertScrollArea->setWidgetResizable(true);

    QWidget *convertContent = new QWidget;
    QVBoxLayout *convertContentLayout = new QVBoxLayout(convertContent);
    convertContentLayout->setContentsMargins(0, 0, 0, 0);
    FormatConvertWidget *convertWidget = new FormatConvertWidget(convertContent);
    convertContentLayout->addWidget(convertWidget);
    convertScrollArea->setWidget(convertContent);

    m_stackWidget->addWidget(convertScrollArea);

    // 索引5：视频压缩（占位）
    QScrollArea *compressScrollArea = new QScrollArea(rightPanel);
    compressScrollArea->setStyleSheet(
        "QScrollArea { background-color: white; border: none; border-radius: 8px; }"
        "QScrollBar:vertical { background: #F0F0F0; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #C0C0C0; border-radius: 4px; min-height: 30px; }"
        );
    compressScrollArea->setWidgetResizable(true);

    QWidget *compressContent = new QWidget;
    QVBoxLayout *compressContentLayout = new QVBoxLayout(compressContent);
    compressContentLayout->setContentsMargins(0, 0, 0, 0);
    CompressWidget *compressWidget = new CompressWidget(compressContent);
    compressContentLayout->addWidget(compressWidget);
    compressScrollArea->setWidget(compressContent);

    m_stackWidget->addWidget(compressScrollArea);
    // ✅ 索引6：音频提取（真正实现）
    QWidget *audioPage = new QWidget(rightPanel);
    audioPage->setStyleSheet("background-color: white; border-radius: 8px;");
    QVBoxLayout *audioLayout = new QVBoxLayout(audioPage);
    AudioExtractWidget *audioWidget = new AudioExtractWidget(audioPage);
    audioLayout->addWidget(audioWidget);
    m_stackWidget->addWidget(audioPage);

    // 索引7：截图工具（占位）
    QScrollArea *screenshotScrollArea = new QScrollArea(rightPanel);
    screenshotScrollArea->setStyleSheet(
        "QScrollArea {"
        "    background-color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "}"
        "QScrollBar:vertical {"
        "    background: #F0F0F0;"
        "    width: 8px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #C0C0C0;"
        "    border-radius: 4px;"
        "    min-height: 30px;"
        "}"
        );
    screenshotScrollArea->setWidgetResizable(true);

    QWidget *screenshotContent = new QWidget;
    QVBoxLayout *screenshotContentLayout = new QVBoxLayout(screenshotContent);
    screenshotContentLayout->setContentsMargins(0, 0, 0, 0);
    ScreenshotWidget *screenshotWidget = new ScreenshotWidget(screenshotContent);
    screenshotContentLayout->addWidget(screenshotWidget);
    screenshotScrollArea->setWidget(screenshotContent);

    m_stackWidget->addWidget(screenshotScrollArea);

    // 索引8：批量处理（占位）
    QWidget *batchPage = new QWidget(rightPanel);
    batchPage->setStyleSheet("background-color: white; border-radius: 8px;");
    QVBoxLayout *batchLayout = new QVBoxLayout(batchPage);
    QLabel *batchLabel = new QLabel("⚡ 批量处理\n\n功能开发中...", batchPage);
    batchLabel->setAlignment(Qt::AlignCenter);
    batchLabel->setStyleSheet("color: #7F8C8D; font-size: 16px;");
    batchLayout->addWidget(batchLabel);
    m_stackWidget->addWidget(batchPage);

    rightLayout->addWidget(m_stackWidget);

    // 底部状态栏
    QHBoxLayout *statusLayout = new QHBoxLayout;
    m_statusLabel = new QLabel("💡 选择一个工具开始使用", rightPanel);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 8px 0;");
    statusLayout->addWidget(m_statusLabel);
    rightLayout->addLayout(statusLayout);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setSizes({160, 690});
    splitter->setHandleWidth(0);
    mainLayout->addWidget(splitter);
}

void VideoToolBox::initConnections()
{
    connect(m_toolList, &QListWidget::currentRowChanged,
            m_stackWidget, &QStackedWidget::setCurrentIndex);

    connect(m_toolList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *item) {
                if (item) {
                    m_statusLabel->setText("💡 " + item->text());
                }
            });
}

void VideoToolBox::setEmbedMode(bool embed)
{
    Q_UNUSED(embed)
}