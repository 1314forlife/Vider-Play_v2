#ifndef DECODER_BASE_H
#define DECODER_BASE_H

#include <QObject>
#include <QString>
#include "src/core/frame.h"

class DecoderBase : public QObject {
    Q_OBJECT
public:
    explicit DecoderBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DecoderBase() = default;

    virtual bool open(const QString& url) = 0;
    virtual void close() = 0;
    virtual bool readFrame(FrameData& frame) = 0;  // 读取一帧

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual double fps() const = 0;

signals:
    void sigError(const QString& error);
};

#endif