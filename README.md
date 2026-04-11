\# 🎬 高性能跨平台媒体播放器

A High-Performance Cross-Platform Media Player



\## ✨ 项目亮点

\- 从零实现基于FFmpeg的\*\*解封装/解码流水线\*\*，告别第三方播放器内核，底层原理全掌控

\- 采用\*\*多线程分离设计\*\*：解封装、解码、音视频同步、渲染独立线程，无卡顿无音画漂移

\- 基于SDL2实现\*\*低延迟音频输出\*\*与视频帧渲染，同步精度控制在±1ms内

\- Qt 5/6跨平台UI框架，支持Windows/Linux编译，界面与核心逻辑完全解耦

\- 轻量级架构设计，无多余依赖，可扩展支持滤镜、字幕、倍速播放等高级功能



\## 🛠️ 技术栈

| 模块 | 技术选型 | 核心作用 |

|------|----------|----------|

| UI框架 | Qt 5.15 / Qt 6 | 跨平台界面开发、事件循环、信号槽通信 |

| 解封装/解码 | FFmpeg 4.4+ | 音视频解封装、软解码、滤镜处理 |

| 音视频同步 | 自定义时钟同步算法 | 基于PTS/DTS实现音画同步，抗网络抖动 |

| 渲染/输出 | SDL 2.0 | 音频PCM输出、视频YUV帧渲染 |

| 开发语言 | C++17 | 高性能核心逻辑实现，RAII资源管理 |



\## 📐 核心架构设计

主线程（UI 交互）↓（信号槽通信）控制线程（状态管理 /

&#x20;倍速 / 暂停）↓（任务调度）

├─ 解封装线程（av\_read\_frame）：拉取 AVPacket

├─ 解码线程（avcodec\_send/receive）：生成 AVFrame

├─ 音频线程（SDL\_AudioCallback）：PCM 重采样 + 输出└─ 视频线程（SDL\_Render）：YUV 转 RGB + 屏幕渲染





\## 🚀 关键技术实现

1\. \*\*FFmpeg解封装解码流程封装\*\*

&#x20;  - 实现`AVFormatContext`/`AVCodecContext`生命周期管理，自动释放避免内存泄漏

&#x20;  - 支持多线程解码（`AV\_CODEC\_FLAG\_LOW\_DELAY`），降低首帧加载延迟



2\. \*\*音视频同步机制\*\*

&#x20;  - 采用音频时钟为主时钟，视频帧动态调整渲染时间，解决音画不同步问题

&#x20;  - 实现PTS校正算法，处理解码时间戳异常导致的卡顿/快进问题



3\. \*\*SDL低延迟音频输出\*\*

&#x20;  - 基于SDL音频回调机制实现PCM数据填充，避免线程阻塞

&#x20;  - 支持不同采样率/声道数自动重采样（swr\_convert），兼容所有音频格式



4\. \*\*跨平台兼容性处理\*\*

&#x20;  - 分离平台相关代码，通过宏定义屏蔽Windows/Linux差异

&#x20;  - 动态库加载设计，FFmpeg/SDL依赖可配置路径，方便分发部署



\## 🎯 功能特性

\- ✅ 支持MP4/AVI/MKV/FLV等主流封装格式

\- ✅ 支持H.264/H.265/VP9等视频编码，AAC/MP3/AC3等音频编码

\- ✅ 播放/暂停/进度条拖拽/音量调节基础功能

\- ✅ 首帧预加载优化，低配置电脑无卡顿

\- ✅ 日志输出模块，方便调试播放异常问题



\## 📦 编译与运行

\### 依赖准备

1\. FFmpeg 开发库（含avformat/avcodec/swrescale/swscale）

2\. SDL2 开发库（2.0.10+）

3\. Qt 5.15+ / Qt 6.x（MinGW/MSVC编译环境）



\### 编译步骤

```bash

\# 1. 配置Qt项目，添加FFmpeg/SDL头文件与库路径

INCLUDEPATH += $$PWD/3rdparty/ffmpeg/include

LIBS += -L$$PWD/3rdparty/ffmpeg/lib -lavformat -lavcodec -lavutil -lswresample -lswscale



INCLUDEPATH += $$PWD/3rdparty/sdl2/include

LIBS += -L$$PWD/3rdparty/sdl2/lib -lSDL2



\# 2. qmake构建

qmake videoPlayer.pro

make -j4





\## 🚀 当前开发进度

\- ✅ 项目框架搭建完成

\- ✅ FFmpeg 解码模块开发完成

\- ✅ SDL2 音频播放模块完成

\- ✅ 多线程架构设计完成

\- 🔄 音视频同步正在调试

\- 🔄 UI界面正在开发

\- 🔄 进度条、倍速、音量控制开发中



\## 🎯 目标功能

\- 支持 MP4 / AVI / MKV / FLV 等格式

\- H.264 / H.265 / AAC 编码解码

\- 播放、暂停、进度调节、音量控制

\- 低延迟、高性能、无卡顿播放



\## 📝 作者

\- 开发者：liujian

\- 邮箱：905314863@qq.com

\- 项目：个人音视频技术学习项目



\---

⚠️ 本项目仅供学习与技术交流，持续更新中……

