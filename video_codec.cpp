#include "video_codec.h"

// MODIFIED: Function signature updated
void video_codec::init_ffmpeg(int SCR_WIDTH, int SCR_HEIGHT, int FPS, const char* ip, int port) {
    // NEW: Construct the output URL for TCP streaming
    std::string out_url = "tcp://" + std::string(ip) + ":" + std::to_string(port);

    // 1. Allocate format context for a network stream
    // MODIFIED: Specify "mpegts" format and use the TCP URL instead of a filename.
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", out_url.c_str());
    if (!ofmt_ctx) {
        std::cerr << "Could not create output context for URL: " << out_url << std::endl;
        exit(1);
    }

    // 2. Find the H.264 encoder (same as before)
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Codec 'libx264' not found" << std::endl;
        exit(1);
    }

    // 3. Create a new stream (same as before)
    stream = avformat_new_stream(ofmt_ctx, codec);
    if (!stream) {
        std::cerr << "Failed allocating output stream" << std::endl;
        exit(1);
    }
    stream->id = ofmt_ctx->nb_streams - 1;

    // 4. Allocate and configure codec context (same as before)
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Failed to allocate the codec context" << std::endl;
        exit(1);
    }

    // Set codec parameters (same as before)
    codec_ctx->codec_id = AV_CODEC_ID_H264;
    codec_ctx->width = SCR_WIDTH;
    codec_ctx->height = SCR_HEIGHT;
    codec_ctx->time_base = { 1, FPS };
    codec_ctx->framerate = { FPS, 1 };
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 1;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);

    // 5. Open the codec (same as before)
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        exit(1);
    }

    // 6. Copy codec parameters to the stream (same as before)
    avcodec_parameters_from_context(stream->codecpar, codec_ctx);

    // 7. Open the output URL
    // MODIFIED: This block now handles opening the network connection.
    // The AVFMT_NOFILE flag check is still valid.
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        // avio_open establishes the TCP connection.
        if (avio_open(&ofmt_ctx->pb, out_url.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output URL " << out_url << std::endl;
            exit(1);
        }
    }

    // 8. Write the stream header
    // MODIFIED: For network streams, this sends the header information needed
    // by the client to start decoding. It's a blocking network call.
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        std::cerr << "Error occurred when writing header to URL" << std::endl;
        exit(1);
    }
    std::cout << "Successfully connected to " << out_url << ". Streaming..." << std::endl;

    // 9. Create SwsContext (same as before)
    sws_ctx = sws_getContext(SCR_WIDTH, SCR_HEIGHT, AV_PIX_FMT_RGB24,
        SCR_WIDTH, SCR_HEIGHT, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    // 10. Allocate the YUV frame (same as before)
    yuv_frame = av_frame_alloc();
    yuv_frame->format = AV_PIX_FMT_YUV420P;
    yuv_frame->width = SCR_WIDTH;
    yuv_frame->height = SCR_HEIGHT;
    av_frame_get_buffer(yuv_frame, 0);
}


// Function to encode a single frame
// NO CHANGES NEEDED HERE
void video_codec::encode_frame(unsigned char* rgb_data, int SCR_HEIGHT, int SCR_WIDTH) {
    // Fill the YUV frame with converted data from the RGB buffer
    uint8_t* inData[1] = { rgb_data + (SCR_HEIGHT - 1) * SCR_WIDTH * 3 };
    int inLinesize[1] = { -SCR_WIDTH * 3 };
    sws_scale(sws_ctx, inData, inLinesize, 0, SCR_HEIGHT, yuv_frame->data, yuv_frame->linesize);

    yuv_frame->pts = frame_count++;

    // Send the frame to the encoder
    int ret = avcodec_send_frame(codec_ctx, yuv_frame);
    if (ret < 0) return;

    // Receive encoded packets from the encoder
    AVPacket* pkt = av_packet_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        else if (ret < 0) {
            std::cerr << "Error during encoding" << std::endl;
            break;
        }

        // Rescale timestamps and write the packet to the network
        av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
        pkt->stream_index = stream->index;

        // This now sends the packet over the TCP connection
        av_interleaved_write_frame(ofmt_ctx, pkt);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}


// Function to finalize the video stream
void video_codec::finish_ffmpeg() {
    // Flush the encoder
    avcodec_send_frame(codec_ctx, NULL);

    // Write the stream trailer. For some protocols, this is important
    // to properly terminate the stream on the client side.
    av_write_trailer(ofmt_ctx);

    // Clean up
    // MODIFIED: Close the network connection and free the context.
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmt_ctx->pb);
    }
    avcodec_free_context(&codec_ctx);
    avformat_free_context(ofmt_ctx);
    sws_freeContext(sws_ctx);
    av_frame_free(&yuv_frame);

    std::cout << "Finished streaming and cleaned up resources." << std::endl;
}
