#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
#include "../ModInterface/inner_struct.h"

class ConfigManager
{
public:
    ConfigManager();
    ~ConfigManager();

private:
    static ConfigManager* singleton; // 设置单例
    config m_para;

public:
    static ConfigManager* instance(); // 获取单例

    int loadConfig(); // 加载配置文件

    config getNetConfig(); // 返回配置 - 网络配置
};

#endif // CONFIGMANAGER_H
