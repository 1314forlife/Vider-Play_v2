#ifndef FORMATCONVERTWIDGET_H
#define FORMATCONVERTWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QCheckBox>

class FormatConvertWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FormatConvertWidget(QWidget *parent = nullptr);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void startConvert();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void readProgress();
    void updateOutputPath();
    void updateFormatInfo();

private:
    void initUI();
    QString buildFFmpegArgs();
    bool checkFFmpeg();
    QString getFileExtension(const QString &format);

    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QComboBox *m_formatCombo;
    QCheckBox *m_keepOriginalCheck;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;
    QLabel *m_infoLabel;
    QLabel *m_fileSizeLabel;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;
    qint64 m_inputSize;
};

#endif // FORMATCONVERTWIDGET_H