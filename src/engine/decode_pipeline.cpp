#include "pipeline.h"
#include "src/plugins/video_decoder/video_decoder.h"

// 视频解码处理链工厂
class VideoDecodePipeline {
public:
    static Pipeline<FrameData> create() {
        Pipeline<FrameData> pipeline;

        // 添加解码处理器
        pipeline.addProcessor([](const FrameData& input, FrameData& output) {
            // 解码逻辑
            return true;
        }, "VideoDecoder");

        // 添加格式转换处理器
        pipeline.addProcessor([](const FrameData& input, FrameData& output) {
            // YUV 格式转换
            return true;
        }, "FormatConverter");

        // 添加同步处理器
        pipeline.addProcessor([](const FrameData& input, FrameData& output) {
            // 音视频同步
            return true;
        }, "SyncProcessor");

        return pipeline;
    }
};