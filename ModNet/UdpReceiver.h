#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include "ModInterface/inner_struct.h"

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <sched.h>
#endif

class UdpReceiver
{

private:
    // 成员变量
    std::thread m_receiver_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_thread_started{false};
    
    int m_socket_fd{-1};
    uint16_t m_port{0};
    std::string m_bind_address{"0.0.0.0"};
    
    // 循环缓冲区
    std::vector<udp_packet_t> m_buffer;
    std::atomic<size_t> m_write_index{0};
    std::atomic<size_t> m_read_index{0};
    size_t m_buffer_size{10000}; // 千兆网速优化：默认10000个数据包，可缓存约118ms的数据
    
    // 线程同步
    std::mutex m_mutex;
    std::condition_variable m_cv;
    
    // CPU核心绑定
    int m_cpu_core{-1}; // -1表示不绑定
    
    // 接收缓冲区大小
    static constexpr size_t RECV_BUFFER_SIZE = 65535;

public:
    UdpReceiver(); 
    ~UdpReceiver(); 
    
    // 禁用拷贝构造和赋值
    UdpReceiver(const UdpReceiver&) = delete;
    UdpReceiver& operator=(const UdpReceiver&) = delete;
    
    // 配置
    bool init(uint16_t port, const std::string& bind_address, int core, size_t size);
    bool start();
    void stop(); 
    bool isRunning() const { return m_running.load(); }
    // 数据获取方法
    bool getNextPacketDirect(const uint8_t** data_ptr, uint32_t* length, udp_packet_info* header,std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    // 数据有无判断
    size_t getAvailablePackets() const;
    void clearBuffer();

private:
    // 内部方法
    void receiverLoop();
    bool initializeSocket();
    void cleanupSocket();
    bool bindToCpuCore(int core);
    
    // 缓冲区操作
    bool writeToBuffer(const uint8_t* data, uint32_t length);
};

#endif // UDPRECEIVER_H 
