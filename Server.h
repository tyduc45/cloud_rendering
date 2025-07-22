/*
 * Server.h
 * Responsibilities:
 * - Listens for a TCP connection on a background thread.
 * - Takes AVPacket objects from a queue.
 * - Muxes packets into an MPEG-TS stream using FFmpeg.
 * - Sends the stream over TCP.
 * - Manages all threading and synchronization.
 */
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class Server {
public:
    Server(std::string listen_ip, int port);
    ~Server();

    // 启动服务器后台线程
    bool start(AVCodecParameters* video_codec_params);
    // 请求关闭服务器
    void stop();

    void setFPS(int fps);

    // 线程安全地将一个数据包放入待发送队列
    void queuePacket(AVPacket* packet);

    // 检查是否有客户端连接
    bool isClientConnected() const;

private:
    // 后台线程的主函数
    void serverLoop();

    // 网络和FFmpeg参数
    std::string m_listen_ip;
    int m_port;
    AVFormatContext* m_ofmtCtx = nullptr;

    // 传输时帧率
    int m_fps;

    // 线程管理
    std::thread m_serverThread;
    std::atomic<bool> m_shutdownRequested{ false };
    std::atomic<bool> m_clientConnected{ false };

    // 线程安全的生产者-消费者队列
    std::queue<AVPacket*> m_packetQueue;
    std::mutex m_queueMutex;
};