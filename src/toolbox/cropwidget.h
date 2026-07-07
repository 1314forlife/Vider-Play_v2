#ifndef CROPWIDGET_H
#define CROPWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QSpinBox>
#include <QCheckBox>

class CropWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CropWidget(QWidget *parent = nullptr);

signals:
    void statusChanged(const QString &message);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void startCrop();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void readProgress();
    void updateOutputPath();

private:
    void initUI();
    QString buildFFmpegArgs();
    bool checkFFmpeg();
    QString formatTime(int seconds);

    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;

    // 时间裁剪
    QSpinBox *m_startHourSpin;
    QSpinBox *m_startMinSpin;
    QSpinBox *m_startSecSpin;
    QSpinBox *m_durationHourSpin;
    QSpinBox *m_durationMinSpin;
    QSpinBox *m_durationSecSpin;

    // 空间裁剪
    QCheckBox *m_enableCropCheck;
    QSpinBox *m_cropXSpin;
    QSpinBox *m_cropYSpin;
    QSpinBox *m_cropWSpin;
    QSpinBox *m_cropHSpin;

    // 快速裁剪（不重新编码）
    QCheckBox *m_fastCropCheck;

    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;
    QLabel *m_infoLabel;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;  // 视频总时长（秒）
};

#endif // CROPWIDGET_H