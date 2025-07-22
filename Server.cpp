
#include "Server.h" // 假设保存在同名头文件中

Server::Server(std::string listen_ip, int port)
    : m_listen_ip(std::move(listen_ip)), m_port(port) {
}

Server::~Server() {
    stop();
}

void Server::setFPS(int fps)
{
    m_fps = fps;
}

bool Server::start(AVCodecParameters* video_codec_params) {
    if (!video_codec_params) return false;

    std::string listen_url = "tcp://" 
        + m_listen_ip + ":" 
        + std::to_string(m_port) 
        + "?listen=1";

    // 初始化网络
    avformat_network_init();

    // 设置输出上下文
    avformat_alloc_output_context2(&m_ofmtCtx, NULL, "mpegts", listen_url.c_str());
    if (!m_ofmtCtx) {
        std::cerr << "Server Error: Could not create output context." << std::endl;
        return false;
    }

    AVStream* stream = avformat_new_stream(m_ofmtCtx, NULL);
    if (!stream) {
        std::cerr << "Server Error: Failed allocating output stream." << std::endl;
        return false;
    }
    avcodec_parameters_copy(stream->codecpar, video_codec_params);
    avcodec_parameters_free(&video_codec_params); // 释放从Encoder获取的参数副本

    // 启动后台线程
    m_serverThread = std::thread(&Server::serverLoop, this);
    return true;
}

void Server::stop() {
    m_shutdownRequested = true;
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    avformat_network_deinit();
}

void Server::queuePacket(AVPacket* packet) {
    if (!m_clientConnected) {
        av_packet_free(&packet); // 如果没有客户端，则丢弃并释放包
        return;
    }
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_packetQueue.push(packet);
}

bool Server::isClientConnected() const {
    return m_clientConnected;
}

void Server::serverLoop() {
    std::cout << "Server thread: Listening on tcp://" << m_listen_ip << ":" << m_port << std::endl;

    // 阻塞等待客户端连接
    if (avio_open(&m_ofmtCtx->pb, m_ofmtCtx->url, AVIO_FLAG_WRITE) < 0) {
        std::cerr << "Server thread: Failed to open listening URL." << std::endl;
        return;
    }

    if (avformat_write_header(m_ofmtCtx, NULL) < 0) {
        std::cerr << "Server thread: Error writing header." << std::endl;
        return;
    }

    std::cout << "Server thread: Client connected!" << std::endl;
    m_clientConnected = true;

    AVStream* out_stream = m_ofmtCtx->streams[0];

    // 消费者循环
    while (!m_shutdownRequested) {
        AVPacket* pkt_to_send = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_packetQueue.empty()) {
                pkt_to_send = m_packetQueue.front();
                m_packetQueue.pop();
            }
        }

        if (pkt_to_send) {
            // 设置时间戳
            av_packet_rescale_ts(pkt_to_send, { 1, m_fps }, out_stream->time_base); 
            pkt_to_send->stream_index = out_stream->index;

            // 发送数据包
            if (av_interleaved_write_frame(m_ofmtCtx, pkt_to_send) < 0) {
                std::cerr << "Server thread: Error writing frame, client may have disconnected." << std::endl;
                m_clientConnected = false;
            }
            av_packet_free(&pkt_to_send);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 队列为空时短暂休眠
        }

        if (!m_clientConnected) {
            // 如果连接断开，可以跳出循环或等待新连接（当前设计为退出）
            break;
        }
    }

    // 清理
    if (m_clientConnected) {
        av_write_trailer(m_ofmtCtx);
    }
    avio_closep(&m_ofmtCtx->pb);
    avformat_free_context(m_ofmtCtx);
    m_ofmtCtx = nullptr;
    m_clientConnected = false;
    std::cout << "Server thread: Shutdown complete." << std::endl;
}
