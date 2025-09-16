#ifndef UDPSTORAGE_H
#define UDPSTORAGE_H

#include <string>
#include <fstream>

#include "ModInterface/inner_struct.h"

class UdpStorage {
public:
    UdpStorage();
    ~UdpStorage();
    
    // 启动存储功能
    bool start(const std::string& storage_path = "./");
    // 停止存储功能
    void stop();    
    // 存储单个UDP报文（直接写入缓冲区）
    bool storePacket(const uint8_t* data, uint32_t length, const udp_packet_info& header);

    // 检查存储组件是否可用
    bool isAvailable() const { return m_is_open && m_file.is_open(); }
    // 获取存储统计信息
    std::string getCurrentFilename() const { return m_current_filename; }
    uint64_t getFileSize() const;
    
private:

private:
    std::ofstream m_file;
    std::string m_current_filename;    
    bool m_is_open;
};

#endif // UDPSTORAGE_H
