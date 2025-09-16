#include "UdpProcess.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>

UdpProcess::UdpProcess()
{
}

UdpProcess::~UdpProcess()
{
    stop();
}

bool UdpProcess::start(uint16_t port, const std::string& bind_address, int cpu_core, const std::string& storage_path)
{
    // 配置接收器
    if (!m_receiver.init(port, bind_address, cpu_core,20000))
    {
        std::cerr << "Failed to configure UDP receiver" << std::endl;
        return false;
    }

    // 启动接收器
    if (!m_receiver.start())
    {
        std::cerr << "Failed to start UDP receiver" << std::endl;
        return false;
    }

    if(!m_storage.start(storage_path))
    {
        std::cerr << "Failed to start Storage mode" << std::endl;
        return false;
    }

    // 启动数据处理线程
    m_running.store(true);
    m_processing_thread = std::thread(&UdpProcess::Loop, this);

    std::cout << "UDP Receiver started on port " << port << " (Optimized for Gigabit network)" << std::endl;
    return true;
}

void UdpProcess::stop()
{
    m_running.store(false);

    if (m_processing_thread.joinable()) {
        m_processing_thread.join();
    }

    m_receiver.stop();
    m_storage.stop();

    std::cout << "UDP Receiver stopped" << std::endl;
}

void UdpProcess::Loop()
{
    const uint8_t* data_ptr = nullptr;
    uint32_t length;
    udp_packet_info head;


    time_t last_print_time = time(NULL);


    while (m_running.load())
    {

        // 从缓冲区获取数据包，直接获取指针避免拷贝
        // 千兆网速优化：减少超时时间，提高处理频率
        if (m_receiver.getNextPacketDirect(&data_ptr, &length, &head, std::chrono::milliseconds(10)))
        {
            m_storage.storePacket(data_ptr, length, head);
            count++;
        }

        // 定期打印统计信息
        time_t now = time(NULL);
        if (now - last_print_time >= 1 )
        {
            static int sec = 0;
            printf("In %d second -- %lu got received -- Total: %lu \n",
                sec++,count - count_last_second, count);
            count_last_second = count;
            last_print_time = now;
        }
    }
}



