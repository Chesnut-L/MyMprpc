#pragma once

#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

// mprpc框架初始化类
// 单例模式
class MprpcApplication
{
public:
    static void Init(int argc, char **argv);
    // 构建唯一的实例
    static MprpcApplication& GetInstance();
    // 获取配置文件
    static MprpcConfig& GetConfig();

private:
    // 需要声明为静态成员变量 在静态成员函数中调用
    static MprpcConfig m_config;

    MprpcApplication();
    ~MprpcApplication(); 
    // 单例模式，只能生成一个对象，将与拷贝构造相关的函数都删除
    MprpcApplication(MprpcApplication&&) = delete;
    MprpcApplication(const MprpcApplication&) = delete;

};