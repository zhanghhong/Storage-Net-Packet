#include "UdpReceiver.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <system_error>

#include "../ModOse/ose_time_sys.h"

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
    typedef SSIZE_T ssize_t;
#else
    #include <sys/select.h>
    #include <errno.h>
    #include <fcntl.h>
#endif

UdpReceiver::UdpReceiver() 
{
    m_buffer.resize(m_buffer_size);
}

UdpReceiver::~UdpReceiver() {
    stop();
}

bool UdpReceiver::init(uint16_t port, const std::string& bind_address, int core, size_t size)
{
    if (m_running.load())
    {
        return false;
    }
    
    // 初始化网络
    m_port = port;
    m_bind_address = bind_address;

    // 初始化绑定核心
    m_cpu_core = core;

    // 初始化循环缓冲区
    m_buffer_size = size;
    m_buffer.resize(m_buffer_size);
    m_write_index.store(0);
    m_read_index.store(0);

    return true;
}

bool UdpReceiver::start() 
{
    // 检查接收器是否已经在运行，避免重复启动
    if (m_running.load()) {
        std::cerr << "UdpReceiver: Already running" << std::endl;
        return false;
    }
    
    // 检查端口号是否已配置，未配置则无法启动
    if (m_port == 0) {
        std::cerr << "UdpReceiver: Port not configured" << std::endl;
        return false;
    }
    
    // 初始化UDP套接字，失败则返回
    if (!initializeSocket()) {
        std::cerr << "UdpReceiver: Failed to initialize socket" << std::endl;
        return false;
    }
    
    // 标记接收器为运行状态
    m_running.store(true);

    // 启动接收线程，执行receiverLoop方法
    m_receiver_thread = std::thread(&UdpReceiver::receiverLoop, this);
    
    // 等待接收线程真正启动（m_thread_started由线程内设置）
    while (!m_thread_started.load() && m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 避免CPU空转
    }
    
    // 如果线程未能正常启动，则返回失败
    if (!m_running.load()) {
        std::cerr << "UdpReceiver: Failed to start receiver thread" << std::endl;
        return false;
    }
    
    std::cout << "UdpReceiver: Started on port " << m_port << std::endl;
    return true;
}

void UdpReceiver::stop() 
{
    if (!m_running.load()) {
        return;
    }
    
    m_running.store(false);
    m_cv.notify_all();
    
    if (m_receiver_thread.joinable()) {
        m_receiver_thread.join();
    }
    
    cleanupSocket();
    m_thread_started.store(false);
    
    std::cout << "UdpReceiver: Stopped" << std::endl;
}

bool UdpReceiver::getNextPacketDirect(const uint8_t** data_ptr, uint32_t* length,
                                      udp_packet_info* header, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // 大于0 则读取
    if (m_cv.wait_for(lock, timeout, [this] { return getAvailablePackets() > 0; })) 
    {
        size_t read_idx = m_read_index.load();
        
        // 检查是否有数据可读
        if (read_idx == m_write_index.load() || !m_buffer[read_idx].valid) {
            return false;
        }
        
        // 直接返回缓冲区数据指针，避免拷贝
        *data_ptr = (const uint8_t*)m_buffer[read_idx].data.data();
        *length = m_buffer[read_idx].length;
        *header = m_buffer[read_idx].info;
        
        m_buffer[read_idx].valid = false;
        
        // 更新读索引
        m_read_index.store((read_idx + 1) % m_buffer_size);
        
        return true;
    }
    
    return false;
}

size_t UdpReceiver::getAvailablePackets() const 
{
    size_t write_idx = m_write_index.load();
    size_t read_idx = m_read_index.load();
    
    if (write_idx >= read_idx) {
        return write_idx - read_idx;
    } else {
        return m_buffer_size - read_idx + write_idx;
    }
}

void UdpReceiver::clearBuffer()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_write_index.store(0);
    m_read_index.store(0);
    
    for (auto& node : m_buffer) {
        node.valid = false;
    }
}

void UdpReceiver::receiverLoop()
{
    m_thread_started.store(true);
    
    // 绑定CPU核心
    if (m_cpu_core >= 0) 
        bindToCpuCore(m_cpu_core);
        
    char recv_buffer[RECV_BUFFER_SIZE];  
    
    while (m_running.load()) 
    {
        // 使用select进行非阻塞接收
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(m_socket_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
#ifdef _WIN32
        int result = select(0, &read_fds, nullptr, nullptr, &timeout);
#else
        int result = select(m_socket_fd + 1, &read_fds, nullptr, nullptr, &timeout);
#endif
        
        if (result < 0) {
            // select错误
            if (m_running.load()) {
                std::cerr << "UdpReceiver: Select error" << std::endl;
            }
            continue;
        } else if (result == 0) {
            // 超时，继续循环
            continue;
        }
        
        // 有数据可读
        if (FD_ISSET(m_socket_fd, &read_fds))
        {
            struct sockaddr_in sender_addr;
            socklen_t addr_len = sizeof(sender_addr);
            
            ssize_t read_length = recvfrom(m_socket_fd, recv_buffer, RECV_BUFFER_SIZE, 0,
                                            (struct sockaddr*)&sender_addr, &addr_len);
            
            if (read_length > 0)
            {
                // 写入缓冲区
                if (writeToBuffer((uint8_t*)recv_buffer, read_length))
                {
                    // 通知等待的线程 有数据可读，进行读取
                    m_cv.notify_one();
                }
                else
                {
                    std::cerr << "UdpReceiver: Failed to write to buffer" << std::endl;
                }
            }
            else if (read_length < 0)
            {
                // 接收错误
                if (m_running.load())
                {
                    std::cerr << "UdpReceiver: Recvfrom error" << std::endl;
                }
            }
        }
    }
}

bool UdpReceiver::initializeSocket()
{
#ifdef _WIN32
    // Windows下初始化Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "UdpReceiver: WSAStartup failed" << std::endl;
        return false;
    }
#endif
    
    // 创建UDP socket
    m_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket_fd < 0) {
        std::cerr << "UdpReceiver: Socket creation failed" << std::endl;
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "UdpReceiver: SO_REUSEADDR failed" << std::endl;
        cleanupSocket();
        return false;
    }
    
    // 设置socket接收缓冲区大小
    // 千兆网速优化：增加缓冲区大小以应对高吞吐量
    // 目标：缓存至少100ms的数据，即85,000包/秒 × 0.1秒 = 8,500包
    int recv_buf_size = 10000 * 1472; // 约14.7MB，可缓存约10,000个包
    if (setsockopt(m_socket_fd, SOL_SOCKET, SO_RCVBUF, (char*)&recv_buf_size, sizeof(recv_buf_size)) < 0) {
        std::cerr << "UdpReceiver: SO_RCVBUF failed" << std::endl;
    }
    
    // 设置非阻塞模式
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(m_socket_fd, FIONBIO, &mode) != 0) {
        std::cerr << "UdpReceiver: Set non-blocking failed" << std::endl;
        cleanupSocket();
        return false;
    }
#else
    int flags = fcntl(m_socket_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(m_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "UdpReceiver: Set non-blocking failed" << std::endl;
        cleanupSocket();
        return false;
    }
#endif
    
    // 绑定地址和端口
    struct sockaddr_in bind_addr;
    std::memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(m_port);
    
    if (m_bind_address == "0.0.0.0") {
        bind_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        bind_addr.sin_addr.s_addr = inet_addr(m_bind_address.c_str());
    }
    
    if (bind(m_socket_fd, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        std::cerr << "UdpReceiver: Bind failed on port " << m_port << std::endl;
        cleanupSocket();
        return false;
    }
    
    return true;
}

void UdpReceiver::cleanupSocket() {
    if (m_socket_fd >= 0) {
#ifdef _WIN32
        closesocket(m_socket_fd);
        WSACleanup();
#else
        close(m_socket_fd);
#endif
        m_socket_fd = -1;
    }
}

bool UdpReceiver::bindToCpuCore(int core) {
#ifdef _WIN32
    // Windows下使用SetThreadAffinityMask
    DWORD_PTR mask = 1ULL << core;
    if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0) {
        std::cerr << "UdpReceiver: Failed to bind to CPU core " << core << std::endl;
        return false;
    }
#else
    // Linux下使用pthread_setaffinity_np
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "UdpReceiver: Failed to bind to CPU core " << core << std::endl;
        return false;
    }
#endif
    
    std::cout << "UdpReceiver: Bound to CPU core " << core << std::endl;
    return true;
}

bool UdpReceiver::writeToBuffer(const uint8_t* data, uint32_t length) 
{
    // 验证数据长度
    if (!data || length == 0 || length > RECV_BUFFER_SIZE) {
        return false;
    }
    
    size_t write_idx = m_write_index.load();
    size_t next_write_idx = (write_idx + 1) % m_buffer_size;
    
    // 检查缓冲区是否已满
    if (next_write_idx == m_read_index.load()) {
        return false; // 缓冲区已满
    }
    
    // 写入数据
    m_buffer[write_idx].data.assign((char*)data, length);
    m_buffer[write_idx].length = length;

    m_buffer[write_idx].valid = true;
    m_buffer[write_idx].info.RecvTime = OSE_TIME_SYS::GetOriginUTCTime();
    
    // 更新写索引
    m_write_index.store(next_write_idx);
    return true;
}
