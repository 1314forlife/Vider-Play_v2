#ifndef FRAME_DATA_H
#define FRAME_DATA_H

// 封装YUV帧数据，替代裸指针数组
struct FrameData
{
    FrameData() = default;  // 默认构造函数（必须）
    FrameData(const FrameData& other) = default;  // 拷贝构造
    FrameData& operator=(const FrameData& other) = default;  // 赋值运算符

    QVector<uint8_t*> data;  // Y/U/V三个平面的指针
    QVector<int> linesize;   // 三个平面的行跨度
    int64_t pts;             // 时间戳（可选，用于同步）
};

// 注册为Qt元类型，支持信号槽跨线程传递
Q_DECLARE_METATYPE(FrameData)

#endif // FRAME_DATA_H
