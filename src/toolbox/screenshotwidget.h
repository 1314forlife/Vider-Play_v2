#ifndef SCREENSHOTWIDGET_H
#define SCREENSHOTWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>

class ScreenshotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenshotWidget(QWidget *parent = nullptr);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void takeScreenshot();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void updateOutputPath();

private:
    void initUI();
    QString buildFFmpegArgs();
    bool checkFFmpeg();
    QString formatTime(int seconds);

    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QSpinBox *m_hourSpin;
    QSpinBox *m_minSpin;
    QSpinBox *m_secSpin;
    QComboBox *m_formatCombo;
    QCheckBox *m_highQualityCheck;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;
    QLabel *m_previewLabel;
    QPixmap m_lastScreenshot;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;
};

#endif // SCREENSHOTWIDGET_H