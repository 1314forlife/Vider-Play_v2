// src/toolbox/transcodewidget.cpp
#include "transcodewidget.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>

TranscodeWidget::TranscodeWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &TranscodeWidget::readProgress);
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TranscodeWidget::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &TranscodeWidget::onProcessError);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TranscodeWidget::updateOutputSuffix);
}

void TranscodeWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // ===== 输入输出区域 =====
    QGroupBox *fileGroup = new QGroupBox("📁 输入输出", this);
    QGridLayout *fileLayout = new QGridLayout(fileGroup);

    fileLayout->addWidget(new QLabel("输入文件:"), 0, 0);
    m_inputEdit = new QLineEdit;
    m_inputEdit->setPlaceholderText("请选择要转码的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    inputBtn->setStyleSheet("color: black;");
    connect(inputBtn, &QPushButton::clicked, this, &TranscodeWidget::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("输出文件:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("转码后的视频保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    outputBtn->setStyleSheet("color: black;");
    connect(outputBtn, &QPushButton::clicked, this, &TranscodeWidget::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    mainLayout->addWidget(fileGroup);

    // ===== 转码设置 =====
    QGroupBox *settingsGroup = new QGroupBox("⚙️ 转码设置", this);
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);

    settingsLayout->addWidget(new QLabel("输出格式:"), 0, 0);
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"MP4", "AVI", "MKV", "MOV", "FLV", "WEBM"});
    settingsLayout->addWidget(m_formatCombo, 0, 1);

    settingsLayout->addWidget(new QLabel("分辨率:"), 1, 0);
    m_resolutionCombo = new QComboBox;
    m_resolutionCombo->addItems({"保持原始", "1920x1080", "1280x720", "854x480", "640x360"});
    settingsLayout->addWidget(m_resolutionCombo, 1, 1);

    settingsLayout->addWidget(new QLabel("视频码率:"), 2, 0);
    m_bitrateCombo = new QComboBox;
    m_bitrateCombo->addItems({"保持原始", "2M", "1M", "500K", "256K"});
    settingsLayout->addWidget(m_bitrateCombo, 2, 1);

    m_fastRemuxCheck = new QCheckBox("⚡ 快速封装（仅换容器，不重新编码）");
    m_fastRemuxCheck->setToolTip("不重新编码，只改变容器格式，速度快且无损画质");
    settingsLayout->addWidget(m_fastRemuxCheck, 3, 0, 1, 2);

    mainLayout->addWidget(settingsGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("▶ 开始转码", this);
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
    connect(m_startBtn, &QPushButton::clicked, this, &TranscodeWidget::startTranscode);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel("💡 选择输入文件后开始转码", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);

    m_formatCombo->setCurrentText("MP4");
}

bool TranscodeWidget::checkFFmpeg()
{
    QProcess process;
    process.start("ffmpeg", {"-version"});
    process.waitForFinished(3000);
    if (process.exitCode() != 0) {
        QMessageBox::critical(this, "错误",
                              "未找到 FFmpeg！\n\n"
                              "请确保 ffmpeg.exe 在系统 PATH 中，\n"
                              "或者放在程序同目录下。");
        return false;
    }
    return true;
}

void TranscodeWidget::selectInputFile()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.webm *.wmv);;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_inputEdit->setText(file);

        QFileInfo info(file);
        QString baseName = info.baseName();

        //使用当前选中的格式作为后缀
        QString format = m_formatCombo->currentText().toLower();
        QString outputPath = info.absolutePath() + "/" + baseName + "_转码." + format;
        m_outputEdit->setText(outputPath);

        m_statusLabel->setText("✅ 已选择: " + info.fileName());

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
        }
    }
}

void TranscodeWidget::selectOutputFile()
{
    QString format = m_formatCombo->currentText().toLower();
    QString filter = format.toUpper() + "文件 (*." + format + ")";

    // ✅ 获取当前输入文件的基础名，自动生成建议文件名
    QString suggestedName;
    if (!m_inputEdit->text().isEmpty()) {
        QFileInfo inputInfo(m_inputEdit->text());
        suggestedName = inputInfo.absolutePath() + "/" + inputInfo.baseName() + "_转码." + format;
    } else {
        suggestedName = "output." + format;
    }

    QString file = QFileDialog::getSaveFileName(
        this,
        "保存视频文件",
        suggestedName,
        filter + ";;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        // ✅ 确保文件有正确的后缀
        if (!file.endsWith("." + format, Qt::CaseInsensitive)) {
            file += "." + format;
        }
        m_outputEdit->setText(file);
    }
}

QString TranscodeWidget::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();
    QString format = m_formatCombo->currentText().toLower();

    // ✅ MKV 格式名修正（FFmpeg 使用 matroska）
    if (format == "mkv") {
        format = "matroska";
    }

    QStringList args;

    // 快速封装模式
    if (m_fastRemuxCheck->isChecked()) {
        args << "-i" << input;
        args << "-c" << "copy";
        // ✅ 不指定 -f，让 FFmpeg 根据文件后缀自动识别格式
        // args << "-f" << format;  // 注释掉这一行
        args << "-y" << output;
        return args.join(" ");
    }

    // 标准转码模式
    args << "-i" << input;

    // 分辨率
    if (m_resolutionCombo->currentIndex() != 0) {
        QString res = m_resolutionCombo->currentText();
        args << "-vf" << ("scale=" + res);
    }

    // 视频编码（H.264）
    args << "-c:v" << "libx264";

    // 码率
    if (m_bitrateCombo->currentIndex() != 0) {
        QString bitrate = m_bitrateCombo->currentText();
        args << "-b:v" << bitrate;
    }

    // 音频编码（AAC）
    args << "-c:a" << "aac";
    args << "-b:a" << "128k";

    // 格式（标准转码模式保留 -f，让 FFmpeg 明确输出格式）
    args << "-f" << format;

    // 覆盖输出
    args << "-y" << output;

    return args.join(" ");
}

void TranscodeWidget::startTranscode()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入文件");
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输出位置");
        return;
    }

    if (!checkFFmpeg()) {
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("⏳ 正在转码... 请稍候");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    m_ffmpegProcess->start("ffmpeg", argList);

    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "错误", "FFmpeg 启动失败！");
        m_startBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("❌ 转码启动失败");
    }
}

void TranscodeWidget::readProgress()
{
    QString output = m_ffmpegProcess->readAllStandardError();

    // 解析进度：time=00:00:05.12
    QRegularExpression regex("time=(\\d{2}):(\\d{2}):(\\d{2})");
    QRegularExpressionMatch match = regex.match(output);

    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        int seconds = match.captured(3).toInt();
        int currentTime = hours * 3600 + minutes * 60 + seconds;

        if (m_duration > 0) {
            int progress = (currentTime * 100) / m_duration;
            progress = qMin(progress, 99);
            m_progressBar->setValue(progress);

            int remaining = m_duration - currentTime;
            m_statusLabel->setText(QString("⏳ 转码中... %1%  剩余: %2秒")
                                       .arg(progress)
                                       .arg(remaining));
        }
    }
}

void TranscodeWidget::updateOutputSuffix()
{
    if (m_outputEdit->text().isEmpty()) {
        return;
    }

    QString currentPath = m_outputEdit->text();
    QFileInfo info(currentPath);

    // 获取当前选中的格式
    QString format = m_formatCombo->currentText().toLower();

    // 检查当前路径是否以 .格式 结尾
    if (!currentPath.endsWith("." + format, Qt::CaseInsensitive)) {
        // 获取不带后缀的路径
        QString basePath = info.absolutePath() + "/" + info.baseName();
        QString newPath = basePath + "." + format;
        m_outputEdit->setText(newPath);

        // 更新状态提示
        m_statusLabel->setText("📝 输出格式已切换: " + format.toUpper());
    }
}

void TranscodeWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("✅ 转码完成！");
        QMessageBox::information(this, "完成", "视频转码已完成！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 转码失败");
        QMessageBox::critical(this, "错误", "转码失败，请检查输入文件是否有效。");
    }
}

void TranscodeWidget::onProcessError(QProcess::ProcessError error)
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