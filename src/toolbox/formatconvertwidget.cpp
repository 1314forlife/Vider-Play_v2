#include "formatconvertwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>

FormatConvertWidget::FormatConvertWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
    , m_inputSize(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &FormatConvertWidget::readProgress);
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FormatConvertWidget::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &FormatConvertWidget::onProcessError);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FormatConvertWidget::updateOutputPath);
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FormatConvertWidget::updateFormatInfo);
}

void FormatConvertWidget::initUI()
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
    m_inputEdit->setPlaceholderText("请选择要转换格式的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    inputBtn->setStyleSheet("color: black;");
    connect(inputBtn, &QPushButton::clicked, this, &FormatConvertWidget::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("输出文件:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("转换后的视频保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    outputBtn->setStyleSheet("color: black;");
    connect(outputBtn, &QPushButton::clicked, this, &FormatConvertWidget::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    // 文件大小信息
    m_fileSizeLabel = new QLabel("", this);
    m_fileSizeLabel->setStyleSheet("color: #7F8C8D; font-size: 11px; padding: 2px 0;");
    fileLayout->addWidget(m_fileSizeLabel, 2, 0, 1, 3);

    mainLayout->addWidget(fileGroup);

    // ===== 格式设置 =====
    QGroupBox *settingsGroup = new QGroupBox("🔄 格式设置", this);
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

    settingsLayout->addWidget(new QLabel("目标格式:"), 0, 0);
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"MP4", "MKV", "AVI", "MOV", "FLV", "WEBM", "TS"});
    settingsLayout->addWidget(m_formatCombo, 0, 1);

    // 格式说明
    m_infoLabel = new QLabel("💡 MP4 是最通用的格式，兼容性最好", this);
    m_infoLabel->setStyleSheet("color: #7F8C8D; font-size: 11px; padding: 5px 0;");
    settingsLayout->addWidget(m_infoLabel, 1, 0, 1, 2);

    // 保持原始编码（默认勾选）
    m_keepOriginalCheck = new QCheckBox("✅ 保持原始编码（快速转换，无损画质）");
    m_keepOriginalCheck->setChecked(true);
    m_keepOriginalCheck->setToolTip("不重新编码，只改变容器格式，速度极快且无损");
    settingsLayout->addWidget(m_keepOriginalCheck, 2, 0, 1, 2);

    QLabel *tipLabel = new QLabel("⚠️ 取消勾选后会重新编码（耗时较长，画质有损）", this);
    tipLabel->setStyleSheet("color: #E67E22; font-size: 11px; padding: 2px 0;");
    settingsLayout->addWidget(tipLabel, 3, 0, 1, 2);

    mainLayout->addWidget(settingsGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("🔄 开始转换", this);
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
    connect(m_startBtn, &QPushButton::clicked, this, &FormatConvertWidget::startConvert);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel("💡 选择视频文件后开始转换", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);

    // 默认选中 MP4
    m_formatCombo->setCurrentText("MP4");
    updateFormatInfo();
}

bool FormatConvertWidget::checkFFmpeg()
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

QString FormatConvertWidget::getFileExtension(const QString &format)
{
    QString f = format.toLower();
    if (f == "mp4") return "mp4";
    if (f == "mkv") return "mkv";
    if (f == "avi") return "avi";
    if (f == "mov") return "mov";
    if (f == "flv") return "flv";
    if (f == "webm") return "webm";
    if (f == "ts") return "ts";
    return "mp4";
}

void FormatConvertWidget::updateFormatInfo()
{
    QString format = m_formatCombo->currentText();
    QMap<QString, QString> infoMap = {
        {"MP4", "最通用格式，所有设备支持"},
        {"MKV", "支持多音轨/字幕，适合存储"},
        {"AVI", "老格式，兼容性好但体积大"},
        {"MOV", "Apple 格式，适合剪辑"},
        {"FLV", "流媒体格式，适合网络"},
        {"WEBM", "开源格式，适合网页"},
        {"TS", "传输流，适合广播"}
    };
    m_infoLabel->setText("💡 " + infoMap.value(format, ""));
}

void FormatConvertWidget::selectInputFile()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.webm *.ts *.wmv);;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_inputEdit->setText(file);
        updateOutputPath();

        QFileInfo info(file);
        m_inputSize = info.size();

        // 显示文件大小
        QString sizeStr;
        if (m_inputSize > 1024 * 1024 * 1024) {
            sizeStr = QString::number(m_inputSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
        } else if (m_inputSize > 1024 * 1024) {
            sizeStr = QString::number(m_inputSize / (1024.0 * 1024.0), 'f', 2) + " MB";
        } else if (m_inputSize > 1024) {
            sizeStr = QString::number(m_inputSize / 1024.0, 'f', 2) + " KB";
        } else {
            sizeStr = QString::number(m_inputSize) + " B";
        }
        m_fileSizeLabel->setText("📊 文件大小: " + sizeStr);

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

void FormatConvertWidget::selectOutputFile()
{
    QString format = m_formatCombo->currentText().toLower();
    QString ext = getFileExtension(format);
    QString filter = format.toUpper() + "文件 (*." + ext + ")";

    QString file = QFileDialog::getSaveFileName(
        this,
        "保存视频文件",
        "",
        filter + ";;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_outputEdit->setText(file);
    }
}

void FormatConvertWidget::updateOutputPath()
{
    if (m_inputEdit->text().isEmpty()) return;

    QFileInfo info(m_inputEdit->text());
    QString baseName = info.baseName();
    QString ext = getFileExtension(m_formatCombo->currentText());

    QString outputPath = info.absolutePath() + "/" + baseName + "_convert." + ext;
    m_outputEdit->setText(outputPath);
}

QString FormatConvertWidget::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();
    QString format = m_formatCombo->currentText().toLower();
    QString ext = getFileExtension(format);

    QStringList args;
    args << "-i" << input;

    if (m_keepOriginalCheck->isChecked()) {
        // 快速模式：不重新编码
        args << "-c" << "copy";
    } else {
        // 重新编码模式：用 H.264/AAC 保证兼容性
        args << "-c:v" << "libx264";
        args << "-c:a" << "aac";
        args << "-b:a" << "128k";
        args << "-preset" << "medium";
    }

    // 指定格式
    if (format == "ts") {
        args << "-f" << "mpegts";
    } else if (format == "webm") {
        args << "-c:v" << "libvpx-vp9";
        args << "-c:a" << "libopus";
        args << "-f" << "webm";
    } else {
        args << "-f" << ext;
    }

    args << "-y" << output;

    return args.join(" ");
}

void FormatConvertWidget::startConvert()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入视频文件");
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输出位置");
        return;
    }

    // 检查输入输出是否相同
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
    m_statusLabel->setText("⏳ 正在转换... 请稍候");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    m_ffmpegProcess->start("ffmpeg", argList);

    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "错误", "FFmpeg 启动失败！");
        m_startBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("❌ 转换启动失败");
    }
}

void FormatConvertWidget::readProgress()
{
    QString output = m_ffmpegProcess->readAllStandardError();

    // 如果快速模式，进度可能不准确，简单处理
    if (m_keepOriginalCheck->isChecked()) {
        // 快速模式很快完成，直接显示 50%
        m_progressBar->setValue(50);
        return;
    }

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
            m_statusLabel->setText(QString("⏳ 转换中... %1%  剩余: %2秒")
                                       .arg(progress)
                                       .arg(remaining));
        }
    }
}

void FormatConvertWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("✅ 格式转换完成！");

        // 显示输出文件大小
        QFileInfo info(m_outputEdit->text());
        if (info.exists()) {
            qint64 size = info.size();
            QString sizeStr;
            if (size > 1024 * 1024 * 1024) {
                sizeStr = QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
            } else if (size > 1024 * 1024) {
                sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 2) + " MB";
            } else if (size > 1024) {
                sizeStr = QString::number(size / 1024.0, 'f', 2) + " KB";
            } else {
                sizeStr = QString::number(size) + " B";
            }
            m_statusLabel->setText(QString("✅ 转换完成！输出大小: %1").arg(sizeStr));
        }

        QMessageBox::information(this, "完成", "格式转换已完成！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 转换失败");
        QMessageBox::critical(this, "错误", "转换失败，请检查输入文件是否有效。");
    }
}

void FormatConvertWidget::onProcessError(QProcess::ProcessError error)
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