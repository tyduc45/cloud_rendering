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

    // ������������̨�߳�
    bool start(AVCodecParameters* video_codec_params);
    // ����رշ�����
    void stop();

    void setFPS(int fps);

    // �̰߳�ȫ�ؽ�һ�����ݰ���������Ͷ���
    void queuePacket(AVPacket* packet);

    // ����Ƿ��пͻ�������
    bool isClientConnected() const;

private:
    // ��̨�̵߳�������
    void serverLoop();

    // �����FFmpeg����
    std::string m_listen_ip;
    int m_port;
    AVFormatContext* m_ofmtCtx = nullptr;

    // ����ʱ֡��
    int m_fps;

    // �̹߳���
    std::thread m_serverThread;
    std::atomic<bool> m_shutdownRequested{ false };
    std::atomic<bool> m_clientConnected{ false };

    // �̰߳�ȫ��������-�����߶���
    std::queue<AVPacket*> m_packetQueue;
    std::mutex m_queueMutex;
};