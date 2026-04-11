QT       += core gui widgets

# 强制C++17标准
CONFIG += c++17
CONFIG += precompile_header
PRECOMPILED_HEADER = src/stdafx.h

DEFINES += _GLIBCXX_USE_C99_STDINT=1
DEFINES += __STDC_LIMIT_MACROS
DEFINES += __STDC_CONSTANT_MACROS
DEFINES += SDL_MAIN_HANDLED
SOURCES += \
    src/core/ffdecoder.cpp \
    src/core/sdlrender.cpp \
    src/core/videoplayer.cpp \
    src/main.cpp \
    src/ui/mainwindow.cpp

HEADERS += \
    src/core/ffdecoder.h \
    src/core/frame_data.h \
    src/core/irender.h \
    src/core/sdlrender.h \
    src/core/videoplayer.h \
    src/stdafx.h \
    src/ui/mainwindow.h

# 第三方库头文件路径
INCLUDEPATH += $$PWD/3rdparty/ffmpeg/include
INCLUDEPATH += $$PWD/3rdparty/sdl2/include

# ================= FFmpeg 库（正确顺序）=================
LIBS += -L$$PWD/3rdparty/ffmpeg/lib \
        -lavformat \
        -lavdevice \
        -lavfilter \
        -lavcodec \
        -lswresample \
        -lswscale \
        -lavutil

# ================= SDL2 库 =================
LIBS += -L$$PWD/3rdparty/sdl2/lib -lSDL2

# ================= Windows 链接修复 =================
win32: LIBS += -lpthread -ldwrite -ldwmapi

# 输出目录
DESTDIR = $$PWD/bin
TARGET = videoPlay

CONFIG += warn_off
CONFIG += release