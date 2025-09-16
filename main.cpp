#include <QCoreApplication>

#include "ModNet/UdpProcess.h"
#include "ModConfig/ConfigManager.h"
#include "ModInterface/inner_struct.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    UdpProcess m_udp_process;

    config conf;
    if(!ConfigManager::instance()->loadConfig())
    {
        conf = ConfigManager::instance()->getNetConfig();
    }
    else
    {
        return a.exec();
    }

    std::string ip = conf.ip;
    int port = conf.port;
    int core = conf.core;
    std::string path = conf.path;

    m_udp_process.start(port, ip, core, path);

    return a.exec();
}
