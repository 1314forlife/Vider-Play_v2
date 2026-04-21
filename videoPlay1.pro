QT += widgets

CONFIG += c++17
CONFIG += warn_off
CONFIG += release

# ================= 当前：视频解码 + 渲染 =================



SOURCES += \
    src/network/network_stream_manager.cpp \
    src/common/logger.cpp \
    src/engine/audio_decode_thread.cpp \
    src/engine/decode_pipeline.cpp \
    src/engine/demux_thread.cpp \
    src/engine/render_prepare_thread.cpp \
    src/engine/video_decode_thread.cpp \
    src/plugins/audio_decoder/audio_decoder.cpp \
    src/plugins/audio_renderer/sdl_audio_renderer.cpp \
    src/plugins/video_renderer/sdl_renderer.cpp \
    src/plugins/video_decoder/video_decoder.cpp \
    src/engine/play_engine.cpp \
    src/theme/theme_manager.cpp \
    src/ui/main_window.cpp \
    src/ui/navigation_widget.cpp \
    src/ui/network_dialog.cpp \
    src/ui/progress_bar.cpp \
    src/ui/title_bar.cpp \
    src/ui/video_widget.cpp \
    main.cpp

# 暂时注释掉（后续音频用）
# SOURCES += \
#     buffer/buffer_monitor.cpp \
#     buffer/frame_buffer.cpp \
#     buffer/packet_buffer.cpp \
#     src/tests/test_renderer.cpp \
#     src/tests/test_sync.cpp \
#     sync/audio_clock.cpp \
#     sync/video_sync.cpp

HEADERS += \
    src/network/network_stream_manager.h \
    src/common/logger.h \
    src/common/thread_safe_queue.h \
    src/core/audio_frame.h \
    src/core/frame.h \
    src/core/packet.h \
    src/core/ring_queue.h \
    src/engine/audio_decode_thread.h \
    src/engine/demux_thread.h \
    src/engine/module_registry.h \
    src/engine/pipeline.h \
    src/engine/render_prepare_thread.h \
    src/engine/thread_registry.h \
    src/engine/video_decode_thread.h \
    src/plugins/audio_decoder/audio_decoder.h \
    src/plugins/audio_decoder/audio_decoder_base.h \
    src/plugins/audio_renderer/audio_renderer.h \
    src/plugins/audio_renderer/sdl_audio_renderer.h \
    src/plugins/video_renderer/video_renderer.h \
    src/plugins/video_renderer/sdl_renderer.h \
    src/plugins/video_decoder/decoder_base.h \
    src/plugins/video_decoder/video_decoder.h \
    src/engine/play_engine.h \
    src/engine/playback_state.h \
    src/resource/av_ptr.h \
    src/resource/resource_guard.h \
    src/resource/sdl_ptr.h \
    src/theme/theme_manager.h \
    src/ui/main_window.h \
    src/ui/navigation_widget.h \
    src/ui/network_dialog.h \
    src/ui/progress_bar.h \
    src/ui/title_bar.h \
    src/ui/video_widget.h \
    src/include/stdafx.h \
    src/stdafx.h




# 暂时注释掉（后续音频用）
# HEADERS += \
#     buffer/buffer_monitor.h \
#     buffer/frame_buffer.h \
#     buffer/packet_buffer.h \
#     src/resource/av_ptr.h \
#     src/resource/resource_guard.h \
#     src/resource/sdl_ptr.h \
#     src/common/ring_buffer.h \
#     src/core/media_info.h \
#     src/tests/test_decoder.h \
#     src/ui/control_bar.h \
#     sync/audio_clock.h \
#     sync/sync_threshold.h \
#     sync/video_sync.h

# 第三方库头文件路径
INCLUDEPATH += $$PWD/3rdparty/ffmpeg/include
INCLUDEPATH += $$PWD/3rdparty/sdl2/include

# FFmpeg 库（视频解码需要）
LIBS += -L$$PWD/3rdparty/ffmpeg/lib \
        -lavformat \
        -lavcodec \
        -lavutil \
        -lswscale \
        -lswresample

# 暂时注释掉（音频用）
# LIBS += -L$$PWD/3rdparty/ffmpeg/lib \
#         -lavdevice \
#         -lavfilter \


# SDL2 库
LIBS += -L$$PWD/3rdparty/sdl2/lib -lSDL2
LIBS += -lopengl32

# Windows 链接修复
win32: LIBS += -lpthread -ldwrite -ldwmapi -lwinpthread

# 输出目录
DESTDIR = $$PWD/bin
TARGET = video_player

# 禁用警告
QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-unused-variable

DISTFILES += \
    src/resource/style.qss \
    src/resource/themes/default.qss \
    src/resource/themes/furina.qss

RESOURCES += \
    resources.qrc \
    resources.qrc