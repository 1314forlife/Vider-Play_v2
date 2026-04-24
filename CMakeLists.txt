cmake_minimum_required(VERSION 3.20)
project(VideoPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_definitions(-Wno-unused-parameter -Wno-unused-variable)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ========== Qt 模块查找 ==========
find_package(Qt6 REQUIRED
    COMPONENTS
    Core
    Widgets
    Network
)

set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg)
set(SDL2_DIR ${CMAKE_SOURCE_DIR}/3rdparty/sdl2)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${FFMPEG_DIR}/include
    ${SDL2_DIR}/include
)

link_directories(
    ${FFMPEG_DIR}/lib
    ${SDL2_DIR}/lib
)

set(SOURCES
    src/network/network_stream_manager.cpp
    src/common/logger.cpp
    src/engine/audio_decode_thread.cpp
    src/engine/decode_pipeline.cpp
    src/engine/demux_thread.cpp
    src/engine/render_prepare_thread.cpp
    src/engine/video_decode_thread.cpp
    src/plugins/audio_decoder/audio_decoder.cpp
    src/plugins/audio_renderer/sdl_audio_renderer.cpp
    src/plugins/video_renderer/sdl_renderer.cpp
    src/plugins/video_decoder/video_decoder.cpp
    src/engine/play_engine.cpp
    src/theme/theme_manager.cpp
    src/ui/furina_lottie.cpp
    src/ui/main_window.cpp
    src/ui/navigation_widget.cpp
    src/ui/network_dialog.cpp
    src/ui/progress_bar.cpp
    src/ui/title_bar.cpp
    src/ui/video_widget.cpp
    main.cpp
)

set(HEADERS
    src/network/network_stream_manager.h
    src/common/logger.h
    src/common/thread_safe_queue.h
    src/core/audio_frame.h
    src/core/frame.h
    src/core/packet.h
    src/core/ring_queue.h
    src/engine/audio_decode_thread.h
    src/engine/demux_thread.h
    src/engine/module_registry.h
    src/engine/pipeline.h
    src/engine/render_prepare_thread.h
    src/engine/thread_registry.h
    src/engine/video_decode_thread.h
    src/plugins/audio_decoder/audio_decoder.h
    src/plugins/audio_decoder/audio_decoder_base.h
    src/plugins/audio_renderer/audio_renderer.h
    src/plugins/audio_renderer/sdl_audio_renderer.h
    src/plugins/video_renderer/video_renderer.h
    src/plugins/video_renderer/sdl_renderer.h
    src/plugins/video_decoder/decoder_base.h
    src/plugins/video_decoder/video_decoder.h
    src/engine/play_engine.h
    src/engine/playback_state.h
    src/resource/av_ptr.h
    src/resource/resource_guard.h
    src/resource/sdl_ptr.h
    src/theme/theme_manager.h
    src/ui/furina_lottie.h
    src/ui/main_window.h
    src/ui/navigation_widget.h
    src/ui/network_dialog.h
    src/ui/progress_bar.h
    src/ui/title_bar.h
    src/ui/video_widget.h
    src/include/stdafx.h
    src/stdafx.h
)

# ========== 资源文件（恢复） ==========
set(RESOURCES
    resources.qrc
)

# ========== 可执行文件 ==========
qt6_add_executable(VideoPlayer ${SOURCES} ${HEADERS} ${RESOURCES}
    src/download/download_task.h
    src/download/download_manager.h
    src/download/download_manager.cpp
    src/ui/download_widget.h
    src/ui/download_widget.cpp
    src/download/SimpleDownloader.cpp
    src/download/SimpleDownloader.h
    src/download/download_manager_v2.h
    src/download/download_manager_v2.cpp
    src/network/rtmp_streamer.h
    src/network/rtmp_streamer.cpp)

target_link_libraries(VideoPlayer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    avformat
    avcodec
    avutil
    swscale
    swresample
    SDL2
    opengl32
    pthread
    dwrite
    dwmapi
    winpthread
)

set_target_properties(VideoPlayer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

target_compile_options(VideoPlayer PRIVATE
    -Wno-unused-parameter
    -Wno-unused-variable
)

file(COPY ${CMAKE_SOURCE_DIR}/src/resource/ DESTINATION ${CMAKE_SOURCE_DIR}/bin/resource/)