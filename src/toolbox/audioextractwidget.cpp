#include "audioextractwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>

AudioExtractWidget::AudioExtractWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &AudioExtractWidget::readProgress);
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AudioExtractWidget::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &AudioExtractWidget::onProcessError);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AudioExtractWidget::updateOutputPath);
    connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AudioExtractWidget::updateOutputPath);
}

void AudioExtractWidget::initUI()
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
    m_inputEdit->setPlaceholderText("请选择要提取音频的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    inputBtn->setStyleSheet("color: black;");
    connect(inputBtn, &QPushButton::clicked, this, &AudioExtractWidget::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("输出音频:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("提取的音频保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    outputBtn->setStyleSheet("color: black;");
    connect(outputBtn, &QPushButton::clicked, this, &AudioExtractWidget::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    mainLayout->addWidget(fileGroup);

    // ===== 音频设置 =====
    QGroupBox *settingsGroup = new QGroupBox("🔊 音频设置", this);
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
    m_formatCombo->addItems({"MP3", "AAC", "WAV", "FLAC", "OGG", "M4A"});
    settingsLayout->addWidget(m_formatCombo, 0, 1);

    settingsLayout->addWidget(new QLabel("音质:"), 1, 0);
    m_qualityCombo = new QComboBox;
    m_qualityCombo->addItems({"高 (320kbps)", "中 (192kbps)", "低 (128kbps)", "极低 (64kbps)"});
    m_qualityCombo->setCurrentIndex(1);
    settingsLayout->addWidget(m_qualityCombo, 1, 1);

    // 提示信息
    m_infoLabel = new QLabel("💡 提示：WAV 和 FLAC 为无损格式，文件较大", this);
    m_infoLabel->setStyleSheet("color: #7F8C8D; font-size: 11px; padding: 5px 0;");
    settingsLayout->addWidget(m_infoLabel, 2, 0, 1, 2);

    mainLayout->addWidget(settingsGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("🎵 开始提取", this);
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
    connect(m_startBtn, &QPushButton::clicked, this, &AudioExtractWidget::startExtract);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel("💡 选择视频文件后开始提取音频", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);
}

bool AudioExtractWidget::checkFFmpeg()
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

QString AudioExtractWidget::formatTime(int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void AudioExtractWidget::selectInputFile()
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

void AudioExtractWidget::selectOutputFile()
{
    QString format = m_formatCombo->currentText().toLower();
    if (format == "aac" || format == "m4a") {
        format = "m4a";
    }
    QString filter = format.toUpper() + "文件 (*." + format + ")";

    QString file = QFileDialog::getSaveFileName(
        this,
        "保存音频文件",
        "",
        filter + ";;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_outputEdit->setText(file);
    }
}

void AudioExtractWidget::updateOutputPath()
{
    if (m_inputEdit->text().isEmpty()) return;

    QFileInfo info(m_inputEdit->text());
    QString baseName = info.baseName();
    QString format = m_formatCombo->currentText().toLower();

    // AAC 和 M4A 都用 m4a 后缀
    if (format == "aac" || format == "m4a") {
        format = "m4a";
    }

    QString outputPath = info.absolutePath() + "/" + baseName + "." + format;
    m_outputEdit->setText(outputPath);
}

QString AudioExtractWidget::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();

    QString format = m_formatCombo->currentText().toLower();

    // 获取码率
    QString bitrate;
    switch (m_qualityCombo->currentIndex()) {
    case 0: bitrate = "320k"; break;  // 高
    case 1: bitrate = "192k"; break;  // 中
    case 2: bitrate = "128k"; break;  // 低
    case 3: bitrate = "64k"; break;   // 极低
    }

    QStringList args;
    args << "-i" << input;
    args << "-vn";  // 不包含视频

    // 根据格式选择编码器
    if (format == "mp3") {
        args << "-acodec" << "libmp3lame";
        args << "-b:a" << bitrate;
    } else if (format == "aac" || format == "m4a") {
        args << "-acodec" << "aac";
        args << "-b:a" << bitrate;
    } else if (format == "wav") {
        args << "-acodec" << "pcm_s16le";
    } else if (format == "flac") {
        args << "-acodec" << "flac";
        args << "-compression_level" << "5";
    } else if (format == "ogg") {
        args << "-acodec" << "libvorbis";
        args << "-b:a" << bitrate;
    }

    args << "-y" << output;

    return args.join(" ");
}

void AudioExtractWidget::startExtract()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入视频文件");
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
    m_statusLabel->setText("⏳ 正在提取音频... 请稍候");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    m_ffmpegProcess->start("ffmpeg", argList);

    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "错误", "FFmpeg 启动失败！");
        m_startBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("❌ 提取失败");
    }
}

void AudioExtractWidget::readProgress()
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
            m_statusLabel->setText(QString("⏳ 提取中... %1%  剩余: %2秒")
                                       .arg(progress)
                                       .arg(remaining));
        }
    }
}

void AudioExtractWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("✅ 音频提取完成！");
        QMessageBox::information(this, "完成", "音频提取已完成！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 提取失败");
        QMessageBox::critical(this, "错误", "提取失败，请检查输入文件是否有效。");
    }
}

void AudioExtractWidget::onProcessError(QProcess::ProcessError error)
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