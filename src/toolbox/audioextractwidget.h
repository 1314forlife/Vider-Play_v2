#ifndef AUDIOEXTRACTWIDGET_H
#define AUDIOEXTRACTWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>

class AudioExtractWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioExtractWidget(QWidget *parent = nullptr);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void startExtract();
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
    QComboBox *m_formatCombo;
    QComboBox *m_qualityCombo;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;
    QLabel *m_infoLabel;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;
};

#endif // AUDIOEXTRACTWIDGET_H