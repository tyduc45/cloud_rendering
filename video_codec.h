#ifndef VIDEO_CODEC_H_
#define VIDEO_CODEC_H_

#include<iostream>
#include<string>
// The 'extern "C"' block is necessary to link C libraries with C++ code.
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}


class video_codec
{
private:
    AVFormatContext* ofmt_ctx;
    AVCodecContext* codec_ctx;
    AVStream* stream;
    SwsContext* sws_ctx;
    AVFrame* yuv_frame;
    long long           frame_count;
    const char* out_filename;
public:
    video_codec(
        AVFormatContext* ofmt_ctx,
        AVCodecContext* codec_ctx,
        AVStream* stream,
        SwsContext* sws_ctx,
        AVFrame* yuv_frame,
        long long frame_count,
        const char* out_filename
    ) :ofmt_ctx(ofmt_ctx), codec_ctx(codec_ctx), stream(stream), sws_ctx(sws_ctx), yuv_frame(yuv_frame), frame_count(frame_count), out_filename(out_filename)
    {}

    void init_ffmpeg(int SCR_WIDTH, int SCR_HEIGHT, int FPS, const char* ip, int port);
    void encode_frame(unsigned char* rgb_data, int SCR_HEIGHT, int SCR_WIDTH);
    void finish_ffmpeg();
};

#endif