#include "screenshotwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>
#include <QPixmap>
#include <QLabel>
#include <QScrollArea>

ScreenshotWidget::ScreenshotWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);

    // ✅ 移除 readyReadStandardError 的连接（截图不需要进度）
    // 只连接 finished 和 errorOccurred
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScreenshotWidget::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &ScreenshotWidget::onProcessError);
}

void ScreenshotWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ===== 输入输出 =====
    QGroupBox *fileGroup = new QGroupBox("📁 输入输出", this);
    fileGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "    border: 1px solid #D0D7DE;"
        "    border-radius: 6px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    background-color: white;"
        "}"
        );
    QGridLayout *fileLayout = new QGridLayout(fileGroup);
    fileLayout->setContentsMargins(15, 25, 15, 15);

    fileLayout->addWidget(new QLabel("输入视频:"), 0, 0);
    m_inputEdit = new QLineEdit;
    m_inputEdit->setPlaceholderText("请选择要截图的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    connect(inputBtn, &QPushButton::clicked, this, &ScreenshotWidget::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("保存位置:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("截图保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    connect(outputBtn, &QPushButton::clicked, this, &ScreenshotWidget::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    mainLayout->addWidget(fileGroup);

    // ===== 截图位置 =====
    QGroupBox *timeGroup = new QGroupBox("⏱️ 截图位置", this);
    timeGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "    border: 1px solid #D0D7DE;"
        "    border-radius: 6px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    background-color: white;"
        "}"
        );
    QHBoxLayout *timeLayout = new QHBoxLayout(timeGroup);
    timeLayout->setContentsMargins(15, 25, 15, 15);

    timeLayout->addWidget(new QLabel("时间点:"));
    timeLayout->addSpacing(10);

    m_hourSpin = new QSpinBox;
    m_hourSpin->setRange(0, 99);
    m_hourSpin->setValue(0);
    m_hourSpin->setFixedWidth(55);
    timeLayout->addWidget(m_hourSpin);
    timeLayout->addWidget(new QLabel("时"));
    timeLayout->addSpacing(5);

    m_minSpin = new QSpinBox;
    m_minSpin->setRange(0, 59);
    m_minSpin->setValue(0);
    m_minSpin->setFixedWidth(55);
    timeLayout->addWidget(m_minSpin);
    timeLayout->addWidget(new QLabel("分"));
    timeLayout->addSpacing(5);

    m_secSpin = new QSpinBox;
    m_secSpin->setRange(0, 59);
    m_secSpin->setValue(10);
    m_secSpin->setFixedWidth(55);
    timeLayout->addWidget(m_secSpin);
    timeLayout->addWidget(new QLabel("秒"));
    timeLayout->addStretch();

    mainLayout->addWidget(timeGroup);

    // ===== 截图设置 =====
    QGroupBox *settingsGroup = new QGroupBox("📷 截图设置", this);
    settingsGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "    border: 1px solid #D0D7DE;"
        "    border-radius: 6px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    background-color: white;"
        "}"
        );
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);
    settingsLayout->setContentsMargins(15, 25, 15, 15);

    settingsLayout->addWidget(new QLabel("输出格式:"), 0, 0);
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"PNG", "JPG", "BMP", "WEBP"});
    settingsLayout->addWidget(m_formatCombo, 0, 1);

    m_highQualityCheck = new QCheckBox("高质量截图（PNG无损）");
    m_highQualityCheck->setChecked(true);
    settingsLayout->addWidget(m_highQualityCheck, 1, 0, 1, 2);

    mainLayout->addWidget(settingsGroup);

    // ===== 预览区域 =====
    QGroupBox *previewGroup = new QGroupBox("🖼️ 预览", this);
    previewGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "    border: 1px solid #D0D7DE;"
        "    border-radius: 6px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    background-color: white;"
        "}"
        );
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    previewLayout->setContentsMargins(15, 25, 15, 15);

    m_previewLabel = new QLabel("截图预览将显示在此处");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #F0F0F0;"
        "    border: 2px dashed #C0C0C0;"
        "    border-radius: 4px;"
        "    min-height: 200px;"
        "    color: #A0A0A0;"
        "    font-size: 14px;"
        "}"
        );
    previewLayout->addWidget(m_previewLabel);

    mainLayout->addWidget(previewGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("📷 截图", this);
    m_startBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #4A90D9;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 10px 30px;"
        "    border-radius: 6px;"
        "    border: none;"
        "}"
        "QPushButton:hover {"
        "    background-color: #5BA0E9;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3A80C9;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #A0B8D0;"
        "}"
        );
    connect(m_startBtn, &QPushButton::clicked, this, &ScreenshotWidget::takeScreenshot);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel("💡 选择视频文件后截图", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);
}

bool ScreenshotWidget::checkFFmpeg()
{
    QProcess process;
    process.start("ffmpeg", {"-version"});
    process.waitForFinished(3000);
    if (process.exitCode() != 0) {
        QMessageBox::critical(this, "错误",
                              "未找到 FFmpeg！\n\n请确保 ffmpeg.exe 在系统 PATH 中。");
        return false;
    }
    return true;
}

QString ScreenshotWidget::formatTime(int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void ScreenshotWidget::selectInputFile()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.webm *.wmv);;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_inputEdit->setText(file);
        updateOutputPath();

        // 获取视频时长
        QProcess probe;
        QStringList args;
        args << "-v" << "error"
             << "-show_entries" << "format=duration"
             << "-of" << "default=noprint_wrappers=1:nokey=1"
             << file;
        probe.start("ffprobe", args);
        probe.waitForFinished(2000);
        QString output = probe.readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            m_duration = output.toDouble();
            m_statusLabel->setText(QString("✅ 视频时长: %1 秒").arg(m_duration));
        }
    }
}

void ScreenshotWidget::selectOutputFile()
{
    QString format = m_formatCombo->currentText().toLower();
    QString filter = format.toUpper() + "文件 (*." + format + ")";

    QString file = QFileDialog::getSaveFileName(
        this,
        "保存截图",
        "",
        filter + ";;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_outputEdit->setText(file);
    }
}

void ScreenshotWidget::updateOutputPath()
{
    if (m_inputEdit->text().isEmpty()) return;

    QFileInfo info(m_inputEdit->text());
    QString baseName = info.baseName();
    QString format = m_formatCombo->currentText().toLower();

    int seconds = m_hourSpin->value() * 3600 +
                  m_minSpin->value() * 60 +
                  m_secSpin->value();

    QString timeStr = formatTime(seconds).replace(":", "-");
    QString outputPath = info.absolutePath() + "/" + baseName + "_" + timeStr + "." + format;
    m_outputEdit->setText(outputPath);
}

QString ScreenshotWidget::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();

    int seconds = m_hourSpin->value() * 3600 +
                  m_minSpin->value() * 60 +
                  m_secSpin->value();

    QString format = m_formatCombo->currentText().toLower();

    QStringList args;
    args << "-i" << input;
    args << "-ss" << QString::number(seconds);
    args << "-vframes" << "1";

    // 高质量截图
    if (m_highQualityCheck->isChecked()) {
        if (format == "jpg" || format == "jpeg") {
            args << "-q:v" << "1";  // 最高质量
        }
        // PNG 默认就是无损
    }

    args << "-y" << output;

    return args.join(" ");
}

void ScreenshotWidget::takeScreenshot()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入视频文件");
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择保存位置");
        return;
    }

    // 检查时间是否超出视频时长
    int seconds = m_hourSpin->value() * 3600 +
                  m_minSpin->value() * 60 +
                  m_secSpin->value();
    if (m_duration > 0 && seconds > m_duration) {
        QMessageBox::warning(this, "提示",
                             QString("截图时间 (%1) 超出视频时长 (%2 秒)")
                                 .arg(formatTime(seconds))
                                 .arg(m_duration));
        return;
    }

    if (!checkFFmpeg()) {
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(50);
    m_statusLabel->setText("⏳ 正在截图...");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    m_ffmpegProcess->start("ffmpeg", argList);

    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "错误", "FFmpeg 启动失败！");
        m_startBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("❌ 截图失败");
    }
}

void ScreenshotWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("✅ 截图完成！");

        // 显示预览
        QPixmap pixmap(m_outputEdit->text());
        if (!pixmap.isNull()) {
            // 缩放预览，保持比例
            QPixmap scaled = pixmap.scaled(400, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_previewLabel->setPixmap(scaled);
            m_previewLabel->setAlignment(Qt::AlignCenter);
        }

        QMessageBox::information(this, "完成", "截图已保存！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 截图失败");
        QMessageBox::critical(this, "错误", "截图失败，请检查输入文件是否有效。");
    }
}

void ScreenshotWidget::onProcessError(QProcess::ProcessError error)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);
    m_statusLabel->setText("❌ FFmpeg 错误");

    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "FFmpeg 启动失败，请确保 ffmpeg.exe 在 PATH 中";
        break;
    case QProcess::Crashed:
        errorMsg = "FFmpeg 进程崩溃";
        break;
    default:
        errorMsg = "未知错误";
    }
    QMessageBox::critical(this, "错误", errorMsg);
}