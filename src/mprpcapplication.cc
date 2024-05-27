#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>
#include <string>

// 静态成员变量需要在类外进行定义
MprpcConfig MprpcApplication::m_config;

void ShowArgHelp()
{
    std::cout << "format: command -i <configfile>" << std::endl;
}

void MprpcApplication::Init(int argc, char **argv)
{
    if (argc < 2)
    {
        // 说明rpc启动时没有传入任何参数
        ShowArgHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        // std::cout << "c" << c << std::endl;
        switch (c)
        {
        case 'i':
            // optarg 是指向选项参数的指针
            config_file = optarg;
            // std::cout << "i" << config_file << std::endl;
            break;
        case '?':
            // 出现了不想要的参数
            // std::cout << "?" << std::endl;
            ShowArgHelp();
            exit(EXIT_FAILURE);
        case ':':
            // 出现-i 但是没有带相应参数
            // std::cout << ":" << std::endl;
            ShowArgHelp();
            exit(EXIT_FAILURE);
        default:
            // std::cout << "default" << std::endl;
            break;
        }
    }

    // 开始加载配置文件 rprserver_ip rprserver_port zookeeper_ip zookeeper_port
    m_config.LoadConfigFile(config_file.c_str());

    // std::cout << "rpcserverip:" << m_config.Load("rpcserverip") << std::endl;
    // std::cout << "rpcserverport:" << m_config.Load("rpcserverport") << std::endl;
    // std::cout << "zookeeperip:" << m_config.Load("zookeeperip") << std::endl;
    // std::cout << "zookeeperport:" << m_config.Load("zookeeperport") << std::endl;
}

// 构建唯一的实例
MprpcApplication &MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

// 获取配置文件
MprpcConfig &MprpcApplication::GetConfig()
{
    return m_config;
}

MprpcApplication::MprpcApplication() {}
MprpcApplication::~MprpcApplication() {}
