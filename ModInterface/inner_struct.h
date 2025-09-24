#ifndef INNER_STRUCT_H
#define INNER_STRUCT_H

#include <string>

#pragma pack(push, 1)
// 包信息
struct udp_packet_info
{
    // 报文接收时间
    double RecvTime;
    // 报文发送端地址
    double SrcAddr;
    // 报文发送端端口
    unsigned short SrcPort;
    // 有效标志
    bool valid;
    // 备用
    bool bak;
    // 报文长度
    uint32_t length;
    // 测试序号
    uint64_t count;
};
#pragma pack(pop)

#pragma pack(push, 1)
// 循环缓冲区节点
struct udp_packet_t
{
    // 报文数据部分
    std::string data;
    // 包信息
    udp_packet_info info;
};
#pragma pack(pop)

struct config
{
    std::string ip;
    int port;
    int core;
    std::string path;
};

#endif // INNER_STRUCT_H
