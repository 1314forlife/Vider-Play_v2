#ifndef STDAFX_H
#define STDAFX_H

// 【第一步】先取消 Qt 冲突宏（必须第一）
#undef slots
#undef signals
#undef emit
#undef foreach

// 【第二步】标准库
#include <cstdint>
#include <cstddef>

// 【第三步】C 语言库：必须包 extern "C"，绝对不能乱！
#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

// 【第四步】恢复 Qt 宏
#define slots Q_SLOTS
#define signals Q_SIGNALS
#define emit Q_EMIT
#define foreach Q_FOREACH

// 【第五步】最后再包含 Qt！！！（顺序绝对不能变）
#include <QtWidgets>
#include <QDebug>
#include <QString>

#endif // STDAFX_H