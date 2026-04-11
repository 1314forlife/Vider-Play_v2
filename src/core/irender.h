#ifndef IRENDER_H
#define IRENDER_H

#include <QObject>  // 必须加
#include <cstdint>
#include <SDL2/SDL.h>
#include "frame_data.h"

class IRender : public QObject
{
public:
    explicit IRender(QObject *parent = nullptr) : QObject(parent) {}

    virtual ~IRender() = default;

    // 初始化：绑定Qt窗口句柄，设置初始分辨率
    virtual bool init(void* winId,int width,int height) = 0;
    // 渲染一帧YUV420P数据（FFmpeg输出的标准格式）
    virtual void render(const FrameData& frame) = 0;
    // 清理资源
    virtual void shutdown() = 0;
    // 设置渲染窗口大小（窗口缩放时调用）
    virtual void setSize(int w, int h) = 0;
};

#endif // IRENDER_H
