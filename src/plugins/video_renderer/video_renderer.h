#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include <QObject>
#include <cstdint>

class VideoRenderer : public QObject {
    Q_OBJECT
public:
    explicit VideoRenderer(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~VideoRenderer() = default;

    virtual bool init(void* windowId, int videoWidth, int videoHeight) = 0;
    virtual bool renderYUV(uint8_t* y, int yLinesize,
                           uint8_t* u, int uLinesize,
                           uint8_t* v, int vLinesize,
                           int64_t pts) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void shutdown() = 0;
};

#endif