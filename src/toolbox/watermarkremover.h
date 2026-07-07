#ifndef WATERMARKREMOVER_H
#define WATERMARKREMOVER_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPixmap>
#include <QRubberBand>
#include <QPoint>

class WatermarkRemover : public QWidget
{
    Q_OBJECT
public:
    explicit WatermarkRemover(QWidget *parent = nullptr);

private slots:
    void selectInputFile();
    void selectOutputFile();
    void startRemove();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void readProgress();
    void updateOutputPath();
    void loadPreview();
    void onPreviewClicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void initUI();
    QString buildFFmpegArgs();
    bool checkFFmpeg();
    QString formatTime(int seconds);

    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QLabel *m_previewLabel;
    QLabel *m_coordLabel;
    QPushButton *m_selectAreaBtn;
    QComboBox *m_methodCombo;
    QSpinBox *m_xSpin;
    QSpinBox *m_ySpin;
    QSpinBox *m_wSpin;
    QSpinBox *m_hSpin;
    QCheckBox *m_previewCheck;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;

    QProcess *m_ffmpegProcess;
    int64_t m_duration;
    QString m_currentVideo;

    // 区域选择相关
    bool m_selecting;
    QPoint m_selectionStart;
    QRubberBand *m_rubberBand;

    // 预览缩放
    double m_scaleX;
    double m_scaleY;
    QPixmap m_previewPixmap;
    int m_previewWidth;
    int m_previewHeight;
};

#endif // WATERMARKREMOVER_H