#include "UdpStorage.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "ModOse/ose_time_sys.h"

UdpStorage::UdpStorage()
{
    m_is_open = false;
}

UdpStorage::~UdpStorage() 
{
    stop();
}

bool UdpStorage::start(const std::string& storage_path)
{
    std::string t_storage_path = storage_path;
    
    // 在启动时创建文件
    // 确保路径以斜杠结尾
    if (!t_storage_path.empty() && t_storage_path.back() != '/' && t_storage_path.back() != '\\')
    {
        t_storage_path += "/";
    }
    m_current_filename = t_storage_path + "data-" + OSE_TIME_SYS::ToString(OSE_TIME_SYS::GetOriginUTCTime()) + ".dat";
    m_file.open(m_current_filename, std::ios::binary | std::ios::app);

    // 打开文件
    if (!m_file.is_open())
    {
        printf("ERROR: Failed to open storage file: %s\n", m_current_filename.c_str());
        return false;
    }
    m_is_open = true;
    printf("Created storage file: %s\n", m_current_filename.c_str());
    
    return true;
}

void UdpStorage::stop() 
{
    // 关闭当前文件
    if (m_is_open && m_file.is_open()) 
    {
        m_file.close();
        m_is_open = false;
        printf("Storage file closed: %s\n", m_current_filename.c_str());
    }
    
    printf("UdpStorage stopped\n");
}

// 存储单个UDP报文
bool UdpStorage::storePacket(const uint8_t* data, uint32_t length, const udp_packet_info& header)
{
    if (!data) {
        return false;
    }

    if (m_is_open && m_file.is_open())
    {
        m_file.write(reinterpret_cast<const char*>(&header),sizeof(udp_packet_info));
        m_file.write(reinterpret_cast<const char*>(data),length);
        m_file.flush();
    }
    
    return true;
}

// 获取文件大小
uint64_t UdpStorage::getFileSize() const
{
    if (m_current_filename.empty()) {
        return 0;
    }
    
    // 获取文件大小
    std::ifstream file(m_current_filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 0;
    }
    
    std::streampos fileSize = file.tellg();
    file.close();
    
    return static_cast<uint64_t>(fileSize);
}
