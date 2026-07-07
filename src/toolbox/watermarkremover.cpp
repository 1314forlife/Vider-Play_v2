#include "watermarkremover.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScrollArea>
#include <QMouseEvent>
#include <QPainter>

WatermarkRemover::WatermarkRemover(QWidget *parent)
    : QWidget(parent)
    , m_duration(0)
    , m_selecting(false)
    , m_scaleX(1.0)
    , m_scaleY(1.0)
    , m_previewWidth(0)
    , m_previewHeight(0)
{
    initUI();
    m_ffmpegProcess = new QProcess(this);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &WatermarkRemover::readProgress);
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &WatermarkRemover::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &WatermarkRemover::onProcessError);
}

void WatermarkRemover::initUI()
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
    m_inputEdit->setPlaceholderText("请选择要去水印的视频文件");
    fileLayout->addWidget(m_inputEdit, 0, 1);
    QPushButton *inputBtn = new QPushButton("浏览");
    inputBtn->setStyleSheet("color: black;");
    connect(inputBtn, &QPushButton::clicked, this, &WatermarkRemover::selectInputFile);
    fileLayout->addWidget(inputBtn, 0, 2);

    fileLayout->addWidget(new QLabel("输出文件:"), 1, 0);
    m_outputEdit = new QLineEdit;
    m_outputEdit->setPlaceholderText("去水印后的视频保存位置");
    fileLayout->addWidget(m_outputEdit, 1, 1);
    QPushButton *outputBtn = new QPushButton("浏览");
    outputBtn->setStyleSheet("color: black;");
    connect(outputBtn, &QPushButton::clicked, this, &WatermarkRemover::selectOutputFile);
    fileLayout->addWidget(outputBtn, 1, 2);

    mainLayout->addWidget(fileGroup);

    // ===== 水印区域选择 =====
    QGroupBox *areaGroup = new QGroupBox("📍 水印区域", this);
    areaGroup->setStyleSheet(
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
    QVBoxLayout *areaLayout = new QVBoxLayout(areaGroup);
    areaLayout->setContentsMargins(15, 25, 15, 15);

    // 预览区域
    m_previewLabel = new QLabel("点击'加载预览'显示视频第一帧\n然后在画面上拖拽选择水印区域");
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
    m_previewLabel->setScaledContents(false);
    m_previewLabel->installEventFilter(this);
    areaLayout->addWidget(m_previewLabel);

    // 坐标显示
    QHBoxLayout *coordLayout = new QHBoxLayout;
    m_coordLabel = new QLabel("水印位置: X:0 Y:0 宽:0 高:0");
    m_coordLabel->setStyleSheet("color: #7F8C8D; font-size: 12px;");
    coordLayout->addWidget(m_coordLabel);
    coordLayout->addStretch();

    m_selectAreaBtn = new QPushButton("🔄 加载预览");
    connect(m_selectAreaBtn, &QPushButton::clicked, this, &WatermarkRemover::loadPreview);
    coordLayout->addWidget(m_selectAreaBtn);

    areaLayout->addLayout(coordLayout);

    // 手动输入坐标
    QGridLayout *manualLayout = new QGridLayout;
    manualLayout->addWidget(new QLabel("X:"), 0, 0);
    m_xSpin = new QSpinBox;
    m_xSpin->setRange(0, 9999);
    m_xSpin->setValue(0);
    manualLayout->addWidget(m_xSpin, 0, 1);

    manualLayout->addWidget(new QLabel("Y:"), 0, 2);
    m_ySpin = new QSpinBox;
    m_ySpin->setRange(0, 9999);
    m_ySpin->setValue(0);
    manualLayout->addWidget(m_ySpin, 0, 3);

    manualLayout->addWidget(new QLabel("宽:"), 1, 0);
    m_wSpin = new QSpinBox;
    m_wSpin->setRange(1, 9999);
    m_wSpin->setValue(200);
    manualLayout->addWidget(m_wSpin, 1, 1);

    manualLayout->addWidget(new QLabel("高:"), 1, 2);
    m_hSpin = new QSpinBox;
    m_hSpin->setRange(1, 9999);
    m_hSpin->setValue(100);
    manualLayout->addWidget(m_hSpin, 1, 3);


    areaLayout->addLayout(manualLayout);

    mainLayout->addWidget(areaGroup);

    // ===== 去水印设置 =====
    QGroupBox *settingsGroup = new QGroupBox("⚙️ 去水印设置", this);
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

    settingsLayout->addWidget(new QLabel("去除算法:"), 0, 0);
    m_methodCombo = new QComboBox;
    m_methodCombo->addItems({"智能填充 (推荐)", "模糊处理", "纹理修复"});
    settingsLayout->addWidget(m_methodCombo, 0, 1);

    m_previewCheck = new QCheckBox("快速预览模式（只处理前10秒）");
    m_previewCheck->setChecked(false);
    settingsLayout->addWidget(m_previewCheck, 1, 0, 1, 2);

    QLabel *tipLabel = new QLabel(
        "💡 在预览画面上拖拽鼠标选择水印区域\n"
        "   也可以直接输入坐标手动设置"
        );
    tipLabel->setStyleSheet("color: #7F8C8D; font-size: 11px; padding: 5px 0;");
    settingsLayout->addWidget(tipLabel, 2, 0, 1, 2);

    mainLayout->addWidget(settingsGroup);

    // ===== 进度和操作 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_startBtn = new QPushButton("🚫 开始去水印", this);
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
    connect(m_startBtn, &QPushButton::clicked, this, &WatermarkRemover::startRemove);
    buttonLayout->addWidget(m_startBtn);

    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel("💡 选择视频文件，加载预览后选择水印区域", this);
    m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 5px 0;");
    mainLayout->addWidget(m_statusLabel);
}

bool WatermarkRemover::checkFFmpeg()
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

void WatermarkRemover::selectInputFile()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.webm *.wmv);;所有文件 (*.*)"
        );
    if (!file.isEmpty()) {
        m_inputEdit->setText(file);
        m_currentVideo = file;
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
            m_statusLabel->setText(QString("✅ 视频时长: %1 秒，点击'加载预览'选择水印区域").arg(m_duration));
        }

        // 自动加载预览
        loadPreview();
    }
}

void WatermarkRemover::selectOutputFile()
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

void WatermarkRemover::updateOutputPath()
{
    if (m_inputEdit->text().isEmpty()) return;

    QFileInfo info(m_inputEdit->text());
    QString baseName = info.baseName();
    QString outputPath = info.absolutePath() + "/" + baseName + "_无水印.mp4";
    m_outputEdit->setText(outputPath);
}

void WatermarkRemover::loadPreview()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择视频文件");
        return;
    }

    m_statusLabel->setText("⏳ 正在加载预览...");

    // 用 ffmpeg 提取第一帧
    QString tempFile = QDir::temp().absoluteFilePath("preview.png");
    QStringList args;
    args << "-i" << m_inputEdit->text();
    args << "-ss" << "1";  // 从第1秒开始，避免黑屏
    args << "-vframes" << "1";
    args << "-y" << tempFile;

    QProcess process;
    process.start("ffmpeg", args);
    process.waitForFinished(5000);

    if (process.exitCode() == 0 && QFile::exists(tempFile)) {
        m_previewPixmap.load(tempFile);
        QFile::remove(tempFile);

        if (!m_previewPixmap.isNull()) {
            // 缩放预览图适应 Label
            int labelWidth = m_previewLabel->width() - 10;
            int labelHeight = m_previewLabel->height() - 10;

            if (labelWidth > 0 && labelHeight > 0) {
                QPixmap scaled = m_previewPixmap.scaled(
                    labelWidth, labelHeight,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                    );
                m_previewLabel->setPixmap(scaled);
                m_previewLabel->setAlignment(Qt::AlignCenter);

                // 计算缩放比例
                m_previewWidth = scaled.width();
                m_previewHeight = scaled.height();
                m_scaleX = (double)m_previewPixmap.width() / m_previewWidth;
                m_scaleY = (double)m_previewPixmap.height() / m_previewHeight;

                m_statusLabel->setText("✅ 预览加载完成，在画面上拖拽选择水印区域");
                m_selectAreaBtn->setText("🔄 重新加载");
                return;
            }
        }
    }

    m_statusLabel->setText("❌ 预览加载失败，请检查视频文件");
}

bool WatermarkRemover::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_previewLabel && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_selecting = true;
            m_selectionStart = mouseEvent->pos();
            if (!m_rubberBand) {
                m_rubberBand = new QRubberBand(QRubberBand::Rectangle, m_previewLabel);
            }
            m_rubberBand->setGeometry(QRect(m_selectionStart, QSize()));
            m_rubberBand->show();
            return true;
        }
    }
    else if (obj == m_previewLabel && event->type() == QEvent::MouseMove) {
        if (m_selecting && m_rubberBand) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QRect rect = QRect(m_selectionStart, mouseEvent->pos()).normalized();
            m_rubberBand->setGeometry(rect);
            return true;
        }
    }
    else if (obj == m_previewLabel && event->type() == QEvent::MouseButtonRelease) {
        if (m_selecting && m_rubberBand) {
            m_selecting = false;
            QRect rect = m_rubberBand->geometry();
            m_rubberBand->hide();

            // 将预览坐标转换为视频原始坐标
            int x = qRound(rect.x() * m_scaleX);
            int y = qRound(rect.y() * m_scaleY);
            int w = qRound(rect.width() * m_scaleX);
            int h = qRound(rect.height() * m_scaleY);

            // 更新 SpinBox
            m_xSpin->setValue(x);
            m_ySpin->setValue(y);
            m_wSpin->setValue(w);
            m_hSpin->setValue(h);

            m_coordLabel->setText(QString("水印位置: X:%1 Y:%2 宽:%3 高:%4").arg(x).arg(y).arg(w).arg(h));
            m_statusLabel->setText(QString("✅ 已选择水印区域: X=%1 Y=%2 宽=%3 高=%4").arg(x).arg(y).arg(w).arg(h));

            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
void WatermarkRemover::startRemove()
{
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输入视频文件");
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输出位置");
        return;
    }

    int x = m_xSpin->value();
    int y = m_ySpin->value();
    int w = m_wSpin->value();
    int h = m_hSpin->value();

    if (w <= 0 || h <= 0) {
        QMessageBox::warning(this, "提示", "请设置有效的水印区域（宽度和高度必须大于0）");
        return;
    }

    if (!checkFFmpeg()) {
        return;
    }

    int ret = QMessageBox::question(this, "确认",
                                    QString("将去除水印区域：X=%1 Y=%2 宽=%3 高=%4\n\n"
                                            "去水印需要重新编码，耗时较长，是否继续？")
                                        .arg(x).arg(y).arg(w).arg(h),
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("⏳ 正在去水印... 请耐心等待");

    QString args = buildFFmpegArgs();
    QStringList argList = args.split(" ", Qt::SkipEmptyParts);

    // ✅ 异步启动，不阻塞 UI
    m_ffmpegProcess->start("ffmpeg", argList);

    // ✅ 移除 waitForStarted，让 QProcess 在后台运行
    // 如果启动失败，会通过 errorOccurred 信号通知

    // ✅ 给用户提示
    m_statusLabel->setText("⏳ 去水印已启动，请等待完成...");
}

QString WatermarkRemover::buildFFmpegArgs()
{
    QString input = m_inputEdit->text();
    QString output = m_outputEdit->text();

    int x = m_xSpin->value();
    int y = m_ySpin->value();
    int w = m_wSpin->value();
    int h = m_hSpin->value();

    QStringList args;
    args << "-i" << input;

    // delogo 滤镜
    args << "-vf" << QString("delogo=x=%1:y=%2:w=%3:h=%4").arg(x).arg(y).arg(w).arg(h);

    // 编码器
    args << "-c:v" << "libx264";
    args << "-c:a" << "aac";
    args << "-b:a" << "128k";
    args << "-preset" << "medium";

    // 快速预览模式
    if (m_previewCheck->isChecked()) {
        args << "-t" << "10";
    }

    args << "-y" << output;

    return args.join(" ");
}

void WatermarkRemover::readProgress()
{
    QString output = m_ffmpegProcess->readAllStandardError();

    QRegularExpression regex("time=(\\d{2}):(\\d{2}):(\\d{2})");
    QRegularExpressionMatch match = regex.match(output);

    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        int seconds = match.captured(3).toInt();
        int currentTime = hours * 3600 + minutes * 60 + seconds;

        int total = m_previewCheck->isChecked() ? 10 : (int)m_duration;

        if (total > 0) {
            int progress = (currentTime * 100) / total;
            progress = qMin(progress, 99);
            m_progressBar->setValue(progress);

            int remaining = total - currentTime;
            m_statusLabel->setText(QString("⏳ 去水印中... %1%  剩余: %2秒")
                                       .arg(progress)
                                       .arg(remaining));

            // ✅ 强制刷新 UI
            QCoreApplication::processEvents();
        }
    }
}

void WatermarkRemover::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (exitCode == 0 && status == QProcess::NormalExit) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("✅ 去水印完成！");
        QMessageBox::information(this, "完成", "去水印已完成！\n\n" + m_outputEdit->text());
    } else {
        m_statusLabel->setText("❌ 去水印失败");
        QMessageBox::critical(this, "错误", "去水印失败，请检查输入文件是否有效。");
    }
}

void WatermarkRemover::onProcessError(QProcess::ProcessError error)
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

void WatermarkRemover::onPreviewClicked()
{
    // 预览点击处理（如果需要可以加载预览）
    loadPreview();
}