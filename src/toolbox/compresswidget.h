#ifndef COMPRESSWIDGET_H
#define COMPRESSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QSlider>
#include <QCheckBox>

class CompressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CompressWidget(QWidget *parent = nullptr);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void startCompress();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void readProgress();
    void updateOutputPath();
    void onCompressLevelChanged(int value);

private:
    void initUI();
    QString buildFFmpegArgs();
    bool checkFFmpeg();
    QString formatFileSize(qint64 size);

    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QComboBox *m_formatCombo;
    QComboBox *m_codecCombo;
    QSlider *m_compressSlider;
    QLabel *m_levelLabel;
    QLabel *m_sizeInfoLabel;
    QCheckBox *m_resolutionCheck;
    QComboBox *m_resolutionCombo;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;
    QLabel *m_predictionLabel;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;
    qint64 m_inputSize;
};

#endif // COMPRESSWIDGET_H