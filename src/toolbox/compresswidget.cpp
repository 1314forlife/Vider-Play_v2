#include "compresswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>

CompressWidget::CompressWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
    , m_inputSize(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &CompressWidget::readProgress);
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CompressWidget::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &CompressWidget::onProcessError);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompressWidget::updateOutputPath);
    connect(m_codecCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompressWidget::updateOutputPath);
    connect(m_compressSlider, &QSlider::valueChanged,
            this, &CompressWidget::onCompressLevelChanged);
}

void CompressWidget::initUI()
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

    fileLayout->addWidget(new QLabel("输入文件:"), 0, 0);
    m_inputEdit = new QLineEdit;
    m_inputEdit->setPlaceholderText("请选择要压缩的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    inputBtn->setStyleSheet("color: black;");
    connect(inputBtn, &QPushButton::clicked, this, &CompressWidget::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("输出文件:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("压缩后的视频保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    outputBtn->setStyleSheet("color: black;");
    connect(outputBtn, &QPushButton::clicked, this, &CompressWidget::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    // 文件大小信息
    m_sizeInfoLabel = new QLabel("", this);
    m_sizeInfoLabel->setStyleSheet("color: #7F8C8D; font-size: 11px; padding: 2px 0;");
    fileLayout->addWidget(m_sizeInfoLabel, 2, 0, 1, 3);

    // 预测大小
    m_predictionLabel = new QLabel("💡 预估压缩后大小: --", this);
    m_predictionLabel->setStyleSheet("color: #27AE60; font-size: 11px; padding: 2px 0;");
    fileLayout->addWidget(m_predictionLabel, 3, 0, 1, 3);

    mainLayout->addWidget(fileGroup);

    // ===== 压缩设置 =====
    QGroupBox *settingsGroup = new QGroupBox("📦 压缩设置", this);
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

    // 输出格式
    settingsLayout->addWidget(new QLabel("输出格式:"), 0, 0);
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"MP4", "MKV", "AVI"});
    settingsLayout->addWidget(m_formatCombo, 0, 1);

    // 编码器
    settingsLayout->addWidget(new QLabel("编码器:"), 1, 0);
    m_codecCombo = new QComboBox;
    m_codecCombo->addItems({"H.264 (兼容性好)", "H.265 (压缩率高)"});
    settingsLayout->addWidget(m_codecCombo, 1, 1);

    // 压缩强度滑块
    settingsLayout->addWidget(new QLabel("压缩强度:"), 2, 0);
    QHBoxLayout *sliderLayout = new QHBoxLayout;
    m_compressSlider = new QSlider(Qt::Horizontal);
    m_compressSlider->setRange(0, 100);
    m_compressSlider->setValue(50);
    m_compressSlider->setTickInterval(10);
    m_compressSlider->setTickPosition(QSlider::TicksBelow);
    sliderLayout->addWidget(m_compressSlider);

    m_levelLabel = new QLabel("中 (50%)");
    m_levelLabel->setFixedWidth(70);
    sliderLayout->addWidget(m_levelLabel);

    settingsLayout->addLayout(sliderLayout, 2, 1);

    // 分辨率选项
    m_resolutionCheck = new QCheckBox("降低分辨率");
    m_resolutionCheck->setChecked(false);
    settingsLayout->addWidget(m_resolutionCheck, 3, 0);

    m_resolutionCombo = new QComboBox;
    m_resolutionCombo->addItems({"保持原始", "1920x1080", "1280x720", "854x480", "640x360"});
    m_resolutionCombo->setEnabled(false);
    settingsLayout->addWidget(m_resolutionCombo, 3, 1);

    connect(m_resolutionCheck, &QCheckBox::toggled, m_resolutionCombo, &QComboBox::setEnabled);

    // 压缩说明
    QLabel *tipLabel = new QLabel(
        "💡 强度越高文件越小，但画质损失也越大\n"
        "   H.265 比 H.264 压缩率高约 50%，但编码更慢"
        );
    tipLabel->setStyleSheet("color: #7F8C8D; font-size: 11px; padding: 5px 0;");
    settingsLayout->addWidget(tipLabel, 4, 0, 1, 2);

    mainLayout->addWidget(settingsGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("📦 开始压缩", this);
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
    connect(m_startBtn, &QPushButton::clicked, this, &CompressWidget::startCompress);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel("💡 选择视频文件后开始压缩", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);
}

bool CompressWidget::checkFFmpeg()
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

QString CompressWidget::formatFileSize(qint64 size)
{
    if (size > 1024 * 1024 * 1024) {
        return QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    } else if (size > 1024 * 1024) {
        return QString::number(size / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (size > 1024) {
        return QString::number(size / 1024.0, 'f', 2) + " KB";
    } else {
        return QString::number(size) + " B";
    }
}

void CompressWidget::onCompressLevelChanged(int value)
{
    QString level;
    if (value < 20) {
        level = "极低";
    } else if (value < 40) {
        level = "低";
    } else if (value < 60) {
        level = "中";
    } else if (value < 80) {
        level = "高";
    } else {
        level = "极高";
    }
    m_levelLabel->setText(QString("%1 (%2%)").arg(level).arg(value));

    // 估算压缩后大小
    if (m_inputSize > 0) {
        // 压缩率 = 1 - (压缩强度 / 100) * 0.7
        double ratio = 1.0 - (value / 100.0) * 0.7;
        double predictedSize = m_inputSize * ratio;
        m_predictionLabel->setText(QString("💡 预估压缩后大小: %1 (约 %2%)")
                                       .arg(formatFileSize(predictedSize))
                                       .arg(qRound(ratio * 100)));
    }
}

void CompressWidget::selectInputFile()
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

        QFileInfo info(file);
        m_inputSize = info.size();
        m_sizeInfoLabel->setText("📊 原始大小: " + formatFileSize(m_inputSize));

        // 触发预测
        onCompressLevelChanged(m_compressSlider->value());

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

void CompressWidget::selectOutputFile()
{
    QString format = m_formatCombo->currentText().toLower();
    QString filter = format.toUpper() + "文件 (*." + format + ")";

    QString file = QFileDialog::getSaveFileName(
        this,
        "保存压缩视频",
        "",
        filter + ";;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_outputEdit->setText(file);
    }
}

void CompressWidget::updateOutputPath()
{
    if (m_inputEdit->text().isEmpty()) return;

    QFileInfo info(m_inputEdit->text());
    QString baseName = info.baseName();
    QString format = m_formatCombo->currentText().toLower();

    QString outputPath = info.absolutePath() + "/" + baseName + "_压缩." + format;
    m_outputEdit->setText(outputPath);
}

QString CompressWidget::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();
    QString format = m_formatCombo->currentText().toLower();
    QString codec = m_codecCombo->currentText();
    int level = m_compressSlider->value();

    // 根据压缩强度计算码率（参考：原始码率按 10Mbps 估算）
    // 码率范围：0.5M - 4M
    double bitrate = 4.0 - (level / 100.0) * 3.5;
    if (bitrate < 0.5) bitrate = 0.5;

    QStringList args;
    args << "-i" << input;

    // 视频编码
    if (codec.contains("H.265")) {
        args << "-c:v" << "libx265";
        args << "-crf" << QString::number(28 + (100 - level) / 10);
    } else {
        args << "-c:v" << "libx264";
        args << "-crf" << QString::number(23 + (100 - level) / 10);
        args << "-preset" << "medium";
    }

    // 码率控制
    args << "-b:v" << QString::number(bitrate, 'f', 1) + "M";
    args << "-maxrate" << QString::number(bitrate * 1.5, 'f', 1) + "M";
    args << "-bufsize" << QString::number(bitrate * 2, 'f', 1) + "M";

    // 分辨率
    if (m_resolutionCheck->isChecked() && m_resolutionCombo->currentIndex() > 0) {
        QString res = m_resolutionCombo->currentText();
        if (res != "保持原始") {
            args << "-vf" << ("scale=" + res);
        }
    }

    // 音频编码
    args << "-c:a" << "aac";
    args << "-b:a" << "128k";

    // 格式
    if (format == "mkv") {
        args << "-f" << "matroska";
    } else {
        args << "-f" << format;
    }

    args << "-y" << output;

    return args.join(" ");
}

void CompressWidget::startCompress()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入视频文件");
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输出位置");
        return;
    }

    if (m_inputEdit->text() == m_outputEdit->text()) {
        QMessageBox::warning(this, "提示", "输入和输出文件不能相同！");
        return;
    }

    if (!checkFFmpeg()) {
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("⏳ 正在压缩... 请耐心等待");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    m_ffmpegProcess->start("ffmpeg", argList);

    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "错误", "FFmpeg 启动失败！");
        m_startBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("❌ 压缩启动失败");
    }
}

void CompressWidget::readProgress()
{
    QString output = m_ffmpegProcess->readAllStandardError();

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
            m_statusLabel->setText(QString("⏳ 压缩中... %1%  剩余: %2秒")
                                       .arg(progress)
                                       .arg(remaining));
        }
    }
}

void CompressWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("✅ 压缩完成！");

        QFileInfo info(m_outputEdit->text());
        if (info.exists()) {
            qint64 outputSize = info.size();
            double ratio = (double)outputSize / m_inputSize * 100;
            m_statusLabel->setText(QString("✅ 压缩完成！%1 → %2 (%.1f%%)")
                                       .arg(formatFileSize(m_inputSize))
                                       .arg(formatFileSize(outputSize))
                                       .arg(ratio));
        }

        QMessageBox::information(this, "完成", "视频压缩已完成！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 压缩失败");
        QMessageBox::critical(this, "错误", "压缩失败，请检查输入文件是否有效。");
    }
}

void CompressWidget::onProcessError(QProcess::ProcessError error)
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