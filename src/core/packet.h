#ifndef PACKET_H
#define PACKET_H

#include <memory>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
}

/**
 * 编码包 - 零拷贝设计
 * 用于在解复用线程和解码线程之间传递
 */
class Packet {
public:
    Packet() = default;

    // 从 AVPacket 构建（移动语义，零拷贝）
    static Packet fromAVPacket(AVPacket* pkt) {
        Packet packet;

        // 🔥 必须复制数据，不能只保存指针
        AVPacket* newPkt = av_packet_alloc();
        av_packet_ref(newPkt, pkt);  // 深拷贝数据

        packet.m_data = std::shared_ptr<AVPacket>(newPkt, [](AVPacket* p) {
            if (p) av_packet_free(&p);
        });
        packet.m_streamIndex = pkt->stream_index;
        packet.m_pts = pkt->pts;
        packet.m_dts = pkt->dts;
        packet.m_duration = pkt->duration;
        packet.m_size = pkt->size;
        return packet;
    }

    // 获取原始 AVPacket 指针（只读）
    AVPacket* get() const { return m_data.get(); }

    int streamIndex() const { return m_streamIndex; }
    int64_t pts() const { return m_pts; }
    int64_t dts() const { return m_dts; }
    int64_t duration() const { return m_duration; }
    int size() const { return m_size; }
    bool isValid() const { return m_data != nullptr; }

private:
    std::shared_ptr<AVPacket> m_data;
    int m_streamIndex = -1;
    int64_t m_pts = AV_NOPTS_VALUE;
    int64_t m_dts = AV_NOPTS_VALUE;
    int64_t m_duration = 0;
    int m_size = 0;
};

#endif