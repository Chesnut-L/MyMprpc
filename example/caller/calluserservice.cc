#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // 整个程序启动后，想使用mprpc框架享受rpc服务调用，一定需要先调用框架的初始化函数，（只初始化一次）
    MprpcApplication::Init(argc, argv);

    /* 演示调用远程发布的rpc方法Login */
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    // rpc方法请求参数
    fixbug::LoginRequest request;
    request.set_name("chesnut");
    request.set_pwd("123456");
    // rpc方法响应
    fixbug::LoginResponse response;
    // 发起rpc方法调用 同步的rpc方法调用过程 MprpcChannel::callMethod
    MprpcController controller;
    stub.Login(&controller, &request, &response, nullptr); // RpcChannel -> RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次 rpc 调用完成，读取调用结果
    if (controller.Failed())
    {
        // rpc调用过程中出现错误
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (response.result().errcode() == 0)
        {
            // 调用成功
            std::cout << "rpc Login response success: " << response.success() << std::endl;
        }
        else
        {
            std::cout << "rpc Login response error: " << response.result().errmsg() << std::endl;
        }
    }

    /* 演示调用远程发布的rpc方法Register */
    // rpc方法请求参数
    fixbug::RegisterRequest register_request;
    register_request.set_id(2000);
    register_request.set_name("chesnut");
    register_request.set_pwd("123456");
    // rpc方法响应
    fixbug::RegisterResponse register_response;
    // 发起rpc方法调用,等待返回结果 同步的rpc方法调用过程 MprpcChannel::callMethod 
    stub.Register(nullptr, &register_request, &register_response, nullptr); // RpcChannel -> RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次 rpc 调用完成，读取调用结果
    if (register_response.result().errcode() == 0)
    {
        // 调用成功
        std::cout << "rpc Register response success: " << register_response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc Register response error: " << register_response.result().errmsg() << std::endl;
    }


    return 0;
}