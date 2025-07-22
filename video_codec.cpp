/*
 * video_codec.cpp
 */
#include "video_codec.h" // 假设保存在同名头文件中

video_codec::video_codec() {}

video_codec::~video_codec() {
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
    }
    if (m_yuvFrame) {
        av_frame_free(&m_yuvFrame);
    }
}

bool video_codec::initialize(int width, int height, int fps) {
    m_width = width;
    m_height = height;

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Encoder Error: Codec 'libx264' not found." << std::endl;
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        std::cerr << "Encoder Error: Failed to allocate the codec context." << std::endl;
        return false;
    }

    m_codecCtx->width = m_width;
    m_codecCtx->height = m_height;
    m_codecCtx->time_base = { 1, fps };
    m_codecCtx->framerate = { fps, 1 };
    m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_codecCtx->gop_size = 12; // A reasonable GOP size
    av_opt_set(m_codecCtx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_codecCtx->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(m_codecCtx, codec, NULL) < 0) {
        std::cerr << "Encoder Error: Could not open codec." << std::endl;
        return false;
    }

    m_swsCtx = sws_getContext(m_width, m_height, AV_PIX_FMT_RGB24, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    m_yuvFrame = av_frame_alloc();
    m_yuvFrame->format = AV_PIX_FMT_YUV420P;
    m_yuvFrame->width = m_width;
    m_yuvFrame->height = m_height;
    av_frame_get_buffer(m_yuvFrame, 32);

    return true;
}

std::vector<AVPacket*> video_codec::encodeFrame(const uint8_t* rgb_data) {
    std::vector<AVPacket*> packets;

    // 转换 RGB -> YUV
    const uint8_t* inData[1] = { rgb_data + (m_height - 1) * m_width * 3 };
    int inLinesize[1] = { -m_width * 3 };
    sws_scale(m_swsCtx, inData, inLinesize, 0, m_height, m_yuvFrame->data, m_yuvFrame->linesize);
    m_yuvFrame->pts = m_frameCount++;

    // 发送帧到编码器
    int ret = avcodec_send_frame(m_codecCtx, m_yuvFrame);
    if (ret < 0) {
        std::cerr << "Encoder Error: Error sending a frame for encoding." << std::endl;
        return packets;
    }

    // 从编码器接收数据包
    while (ret >= 0) {
        AVPacket* pkt = av_packet_alloc();
        ret = avcodec_receive_packet(m_codecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&pkt);
            break;
        } else if (ret < 0) {
            std::cerr << "Encoder Error: Error during encoding." << std::endl;
            av_packet_free(&pkt);
            break;
        }
        packets.push_back(pkt);
    }
    return packets;
}

AVCodecParameters* video_codec::getCodecParameters() const {
    if (!m_codecCtx) return nullptr;
    AVCodecParameters* params = avcodec_parameters_alloc();
    avcodec_parameters_from_context(params, m_codecCtx);
    return params;
}
