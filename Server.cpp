
#include "Server.h" // ���豣����ͬ��ͷ�ļ���

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

    // ��ʼ������
    avformat_network_init();

    // �������������
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
    avcodec_parameters_free(&video_codec_params); // �ͷŴ�Encoder��ȡ�Ĳ�������

    // ������̨�߳�
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
        av_packet_free(&packet); // ���û�пͻ��ˣ��������ͷŰ�
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

    // �����ȴ��ͻ�������
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

    // ������ѭ��
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
            // ����ʱ���
            av_packet_rescale_ts(pkt_to_send, { 1, m_fps }, out_stream->time_base); 
            pkt_to_send->stream_index = out_stream->index;

            // �������ݰ�
            if (av_interleaved_write_frame(m_ofmtCtx, pkt_to_send) < 0) {
                std::cerr << "Server thread: Error writing frame, client may have disconnected." << std::endl;
                m_clientConnected = false;
            }
            av_packet_free(&pkt_to_send);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // ����Ϊ��ʱ��������
        }

        if (!m_clientConnected) {
            // ������ӶϿ�����������ѭ����ȴ������ӣ���ǰ���Ϊ�˳���
            break;
        }
    }

    // ����
    if (m_clientConnected) {
        av_write_trailer(m_ofmtCtx);
    }
    avio_closep(&m_ofmtCtx->pb);
    avformat_free_context(m_ofmtCtx);
    m_ofmtCtx = nullptr;
    m_clientConnected = false;
    std::cout << "Server thread: Shutdown complete." << std::endl;
}
