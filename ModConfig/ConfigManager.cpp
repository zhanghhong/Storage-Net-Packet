#include "ConfigManager.h"
#include "../ModOse/tinyXML/tinyxml2.h"

ConfigManager* ConfigManager::singleton = nullptr;

ConfigManager::ConfigManager()
{

}

ConfigManager::~ConfigManager()
{

}

ConfigManager* ConfigManager::instance()
{
    if (singleton == nullptr)
        singleton = new ConfigManager();
    return singleton;
}

int ConfigManager::loadConfig()
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.LoadFile("./Config/comm_conf.xml");
    if(error != tinyxml2::XML_SUCCESS)
        return 1;

    const char * str;
    tinyxml2::XMLElement* FNode;
    tinyxml2::XMLElement* element;
    
    // 加载网络配置
    FNode = doc.FirstChildElement( "PARA" );
    element = FNode->FirstChildElement("SRC");
    if(element->QueryStringAttribute( "ip", &str ) == tinyxml2::XML_SUCCESS)
        m_para.ip = str;
    else
        return 2;
    if(element->QueryIntAttribute( "port", &m_para.port ))
        return 3;

    element = FNode->FirstChildElement("CORE");
    if(element->QueryIntAttribute( "var", &m_para.core ))
        return 4;

    element = FNode->FirstChildElement("PATH");
    if(element->QueryStringAttribute( "var", &str ) == tinyxml2::XML_SUCCESS)
        m_para.path = str;
    else
        return 6;

    return 0;
}


config ConfigManager::getNetConfig()
{
    return m_para;
}
