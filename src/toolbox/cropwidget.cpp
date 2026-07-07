#include "cropwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>

CropWidget::CropWidget(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &CropWidget::readProgress);
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CropWidget::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &CropWidget::onProcessError);

    // 连接信号，自动更新输出路径
    connect(m_startHourSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CropWidget::updateOutputPath);
    connect(m_startMinSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CropWidget::updateOutputPath);
    connect(m_startSecSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CropWidget::updateOutputPath);
    connect(m_durationHourSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CropWidget::updateOutputPath);
    connect(m_durationMinSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CropWidget::updateOutputPath);
    connect(m_durationSecSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CropWidget::updateOutputPath);
}

void CropWidget::initUI()
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
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
        );
    QGridLayout *fileLayout = new QGridLayout(fileGroup);
    fileLayout->setContentsMargins(15, 25, 15, 15);

    fileLayout->addWidget(new QLabel("输入文件:"), 0, 0);
    m_inputEdit = new QLineEdit;
    m_inputEdit->setPlaceholderText("请选择要裁剪的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    connect(inputBtn, &QPushButton::clicked, this, &CropWidget::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("输出文件:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("裁剪后的视频保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    connect(outputBtn, &QPushButton::clicked, this, &CropWidget::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    mainLayout->addWidget(fileGroup);

    // ===== 时间裁剪 =====
    QGroupBox *timeGroup = new QGroupBox("⏱️ 时间裁剪", this);
    timeGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
        );
    QGridLayout *timeLayout = new QGridLayout(timeGroup);
    timeLayout->setContentsMargins(15, 25, 15, 15);

    timeLayout->addWidget(new QLabel("开始时间:"), 0, 0);
    QHBoxLayout *startLayout = new QHBoxLayout;
    m_startHourSpin = new QSpinBox;
    m_startHourSpin->setRange(0, 99);
    m_startHourSpin->setValue(0);
    m_startHourSpin->setFixedWidth(50);
    startLayout->addWidget(m_startHourSpin);
    startLayout->addWidget(new QLabel("时"));

    m_startMinSpin = new QSpinBox;
    m_startMinSpin->setRange(0, 59);
    m_startMinSpin->setValue(0);
    m_startMinSpin->setFixedWidth(50);
    startLayout->addWidget(m_startMinSpin);
    startLayout->addWidget(new QLabel("分"));

    m_startSecSpin = new QSpinBox;
    m_startSecSpin->setRange(0, 59);
    m_startSecSpin->setValue(0);
    m_startSecSpin->setFixedWidth(50);
    startLayout->addWidget(m_startSecSpin);
    startLayout->addWidget(new QLabel("秒"));
    startLayout->addStretch();

    timeLayout->addLayout(startLayout, 0, 1);

    timeLayout->addWidget(new QLabel("持续时长:"), 1, 0);
    QHBoxLayout *durationLayout = new QHBoxLayout;
    m_durationHourSpin = new QSpinBox;
    m_durationHourSpin->setRange(0, 99);
    m_durationHourSpin->setValue(0);
    m_durationHourSpin->setFixedWidth(50);
    durationLayout->addWidget(m_durationHourSpin);
    durationLayout->addWidget(new QLabel("时"));

    m_durationMinSpin = new QSpinBox;
    m_durationMinSpin->setRange(0, 59);
    m_durationMinSpin->setValue(0);
    m_durationMinSpin->setFixedWidth(50);
    durationLayout->addWidget(m_durationMinSpin);
    durationLayout->addWidget(new QLabel("分"));

    m_durationSecSpin = new QSpinBox;
    m_durationSecSpin->setRange(0, 59);
    m_durationSecSpin->setValue(30);
    m_durationSecSpin->setFixedWidth(50);
    durationLayout->addWidget(m_durationSecSpin);
    durationLayout->addWidget(new QLabel("秒"));
    durationLayout->addStretch();

    timeLayout->addLayout(durationLayout, 1, 1);

    mainLayout->addWidget(timeGroup);

    // ===== 空间裁剪 =====
    QGroupBox *spaceGroup = new QGroupBox("📐 空间裁剪（可选）", this);
    spaceGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
        );
    QGridLayout *spaceLayout = new QGridLayout(spaceGroup);
    spaceLayout->setContentsMargins(15, 25, 15, 15);

    m_enableCropCheck = new QCheckBox("启用空间裁剪");
    spaceLayout->addWidget(m_enableCropCheck, 0, 0, 1, 4);

    spaceLayout->addWidget(new QLabel("左上角 X:"), 1, 0);
    m_cropXSpin = new QSpinBox;
    m_cropXSpin->setRange(0, 9999);
    m_cropXSpin->setValue(0);
    spaceLayout->addWidget(m_cropXSpin, 1, 1);

    spaceLayout->addWidget(new QLabel("Y:"), 1, 2);
    m_cropYSpin = new QSpinBox;
    m_cropYSpin->setRange(0, 9999);
    m_cropYSpin->setValue(0);
    spaceLayout->addWidget(m_cropYSpin, 1, 3);

    spaceLayout->addWidget(new QLabel("裁剪宽度:"), 2, 0);
    m_cropWSpin = new QSpinBox;
    m_cropWSpin->setRange(1, 9999);
    m_cropWSpin->setValue(1280);
    spaceLayout->addWidget(m_cropWSpin, 2, 1);

    spaceLayout->addWidget(new QLabel("高度:"), 2, 2);
    m_cropHSpin = new QSpinBox;
    m_cropHSpin->setRange(1, 9999);
    m_cropHSpin->setValue(720);
    spaceLayout->addWidget(m_cropHSpin, 2, 3);

    mainLayout->addWidget(spaceGroup);

    // ===== 裁剪选项 =====
    QGroupBox *optionGroup = new QGroupBox("⚙️ 裁剪选项", this);
    optionGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    padding-top: 15px;"
        "    margin-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
        );
    QVBoxLayout *optionLayout = new QVBoxLayout(optionGroup);
    optionLayout->setContentsMargins(15, 25, 15, 15);

    m_fastCropCheck = new QCheckBox("⚡ 快速裁剪（不重新编码，仅时间裁剪时可用）");
    m_fastCropCheck->setToolTip("只裁剪时间时不重新编码，速度快且无损画质");
    optionLayout->addWidget(m_fastCropCheck);

    // 提示信息
    m_infoLabel = new QLabel("💡 提示：启用空间裁剪或改变分辨率时需要重新编码", this);
    m_infoLabel->setStyleSheet("color: #7F8C8D; font-size: 11px;");
    optionLayout->addWidget(m_infoLabel);

    mainLayout->addWidget(optionGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("✂️ 开始裁剪", this);
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
    connect(m_startBtn, &QPushButton::clicked, this, &CropWidget::startCrop);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    // 状态标签
    m_statusLabel = new QLabel("💡 选择输入文件后开始裁剪", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);
}

bool CropWidget::checkFFmpeg()
{
    QProcess process;
    process.start("ffmpeg", {"-version"});
    process.waitForFinished(3000);
    if (process.exitCode() != 0) {
        QMessageBox::critical(this, "错误",
                              "未找到 FFmpeg！\n\n"
                              "请确保 ffmpeg.exe 在系统 PATH 中。");
        return false;
    }
    return true;
}

QString CropWidget::formatTime(int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void CropWidget::selectInputFile()
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

void CropWidget::selectOutputFile()
{
    QString file = QFileDialog::getSaveFileName(
        this,
        "保存视频文件",
        "",
        "MP4文件 (*.mp4);;AVI文件 (*.avi);;MKV文件 (*.mkv)"
        );
    if (!file.isEmpty()) {
        m_outputEdit->setText(file);
    }
}

void CropWidget::updateOutputPath()
{
    if (m_inputEdit->text().isEmpty()) return;

    QFileInfo info(m_inputEdit->text());
    QString baseName = info.baseName();

    // 构建时间后缀
    int startSec = m_startHourSpin->value() * 3600 +
                   m_startMinSpin->value() * 60 +
                   m_startSecSpin->value();
    int durationSec = m_durationHourSpin->value() * 3600 +
                      m_durationMinSpin->value() * 60 +
                      m_durationSecSpin->value();

    QString timeSuffix = QString("_%1_%2")
                             .arg(formatTime(startSec).replace(":", "-"))
                             .arg(formatTime(durationSec).replace(":", "-"));

    QString outputPath = info.absolutePath() + "/" + baseName + timeSuffix + ".mp4";
    m_outputEdit->setText(outputPath);
}

QString CropWidget::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();

    // 计算时间参数
    int startSec = m_startHourSpin->value() * 3600 +
                   m_startMinSpin->value() * 60 +
                   m_startSecSpin->value();
    int durationSec = m_durationHourSpin->value() * 3600 +
                      m_durationMinSpin->value() * 60 +
                      m_durationSecSpin->value();

    // 检查是否启用空间裁剪
    bool enableCrop = m_enableCropCheck->isChecked();
    bool fastMode = m_fastCropCheck->isChecked() && !enableCrop;

    QStringList args;
    args << "-i" << input;

    // 时间裁剪
    if (startSec > 0) {
        args << "-ss" << QString::number(startSec);
    }
    if (durationSec > 0) {
        args << "-t" << QString::number(durationSec);
    }

    // 空间裁剪
    if (enableCrop) {
        int x = m_cropXSpin->value();
        int y = m_cropYSpin->value();
        int w = m_cropWSpin->value();
        int h = m_cropHSpin->value();
        args << "-vf" << QString("crop=%1:%2:%3:%4").arg(w).arg(h).arg(x).arg(y);

        // 空间裁剪时必须重新编码
        args << "-c:v" << "libx264";
        args << "-c:a" << "aac";
        args << "-b:a" << "128k";
    } else if (fastMode) {
        // 快速模式：只裁剪时间，不重新编码
        args << "-c" << "copy";
    } else {
        // 普通模式：重新编码（保证兼容性）
        args << "-c:v" << "libx264";
        args << "-c:a" << "aac";
        args << "-b:a" << "128k";
    }

    // 覆盖输出
    args << "-y" << output;

    return args.join(" ");
}

void CropWidget::startCrop()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入文件");
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输出位置");
        return;
    }

    // 检查持续时间
    int durationSec = m_durationHourSpin->value() * 3600 +
                      m_durationMinSpin->value() * 60 +
                      m_durationSecSpin->value();
    if (durationSec <= 0) {
        QMessageBox::warning(this, "提示", "请设置有效的持续时长（大于0秒）");
        return;
    }

    if (!checkFFmpeg()) {
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("⏳ 正在裁剪... 请稍候");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    m_ffmpegProcess->start("ffmpeg", argList);

    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "错误", "FFmpeg 启动失败！");
        m_startBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("❌ 裁剪启动失败");
    }
}

void CropWidget::readProgress()
{
    QString output = m_ffmpegProcess->readAllStandardError();

    // 解析进度
    QRegularExpression regex("time=(\\d{2}):(\\d{2}):(\\d{2})");
    QRegularExpressionMatch match = regex.match(output);

    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        int seconds = match.captured(3).toInt();
        int currentTime = hours * 3600 + minutes * 60 + seconds;

        int durationSec = m_durationHourSpin->value() * 3600 +
                          m_durationMinSpin->value() * 60 +
                          m_durationSecSpin->value();

        if (durationSec > 0) {
            int progress = (currentTime * 100) / durationSec;
            progress = qMin(progress, 99);
            m_progressBar->setValue(progress);

            int remaining = durationSec - currentTime;
            m_statusLabel->setText(QString("⏳ 裁剪中... %1%  剩余: %2秒")
                                       .arg(progress)
                                       .arg(remaining));
        }
    }
}

void CropWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("裁剪完成！");
        QMessageBox::information(this, "完成", "视频裁剪已完成！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 裁剪失败");
        QMessageBox::critical(this, "错误", "裁剪失败，请检查输入文件是否有效。");
    }
}

void CropWidget::onProcessError(QProcess::ProcessError error)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);
    m_statusLabel->setText("FFmpeg 错误");

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