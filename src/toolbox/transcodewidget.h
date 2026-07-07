// src/toolbox/transcodewidget.h
#ifndef TRANSCODEWIDGET_H
#define TRANSCODEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QCheckBox>
#include <QProcess>

class TranscodeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TranscodeWidget(QWidget *parent = nullptr);

signals:
    void statusChanged(const QString &message);
    void progressUpdated(int percent);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void startTranscode();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void readProgress();
    void updateOutputSuffix();

private:
    void initUI();
    QString buildFFmpegArgs();
    bool checkFFmpeg();

    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QComboBox *m_formatCombo;
    QComboBox *m_resolutionCombo;
    QComboBox *m_bitrateCombo;
    QCheckBox *m_fastRemuxCheck;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;  // 视频总时长（秒）
};

#endif // TRANSCODEWIDGET_H