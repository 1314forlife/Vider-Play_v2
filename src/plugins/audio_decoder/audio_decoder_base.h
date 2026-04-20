#ifndef AUDIO_DECODER_BASE_H
#define AUDIO_DECODER_BASE_H

#include <QObject>
#include <QString>

class AudioFrame;

class AudioDecoderBase : public QObject {
    Q_OBJECT
public:
    explicit AudioDecoderBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~AudioDecoderBase() = default;

    virtual bool open(const QString& url) = 0;
    virtual void close() = 0;
    virtual bool readFrame(AudioFrame& frame) = 0;
    virtual bool isEOF() const = 0;

    virtual int sampleRate() const = 0;
    virtual int channels() const = 0;
    virtual int64_t duration() const = 0;
    virtual bool isOpen() const = 0;

signals:
    void sigError(const QString& error);
};

#endif