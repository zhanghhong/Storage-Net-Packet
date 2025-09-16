#ifndef UDPPROCESS_H
#define UDPPROCESS_H

#include <QVector>
#include "UdpReceiver.h"
#include "UdpStorage.h"

class UdpProcess
{
public:
    UdpProcess();
    ~UdpProcess();

public:
    bool start(uint16_t port, const std::string& bind_address, int cpu_core = -1, const std::string& storage_path = "./data/");
    void stop();

private:
    void Loop();

private:
    UdpReceiver m_receiver;
    UdpStorage m_storage;
    std::thread m_processing_thread;
    std::atomic<bool> m_running{false};

    // 统计报数
    uint64_t count = 0;
    uint64_t count_last_second = 0;
};

#endif // UDPPROCESS_H
