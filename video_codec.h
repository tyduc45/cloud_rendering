/*
 * video_codec.h
 * Responsibilities:
 * - Initializes an H.264 encoder.
 * - Converts raw RGB frames to YUV.
 * - Encodes YUV frames into AVPacket objects.
 * - Does NOT handle networking, threading, or muxing.
 */


#ifndef VIDEO_CODEC_H_
#define VIDEO_CODEC_H_

#include<iostream>
#include<string>
#include <vector>
#include <memory>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

class video_codec {
public:
    video_codec();
    ~video_codec();

    // ��ʼ��������
    bool initialize(int width, int height, int fps);

    // ����һ֡ͼ�񣬷���һ���������������ݰ�
    std::vector<AVPacket*> encodeFrame(const uint8_t* rgb_data);

    // ��ȡ��������������Muxer��Serverʹ��
    AVCodecParameters* getCodecParameters() const;

private:
    AVCodecContext* m_codecCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    AVFrame* m_yuvFrame = nullptr;
    int m_width = 0;
    int m_height = 0;
    long long m_frameCount = 0;
};
#endif