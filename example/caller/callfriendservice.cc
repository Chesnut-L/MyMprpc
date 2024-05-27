#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"


int main(int argc, char **argv)
{
    // 整个程序启动后，想使用mprpc框架享受rpc服务调用，一定需要先调用框架的初始化函数，（只初始化一次）
    MprpcApplication::Init(argc, argv);

    /*
        演示调用远程发布的rpc方法
    */
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    // rpc方法请求参数
    fixbug::GetFriendListRequest request;
    request.set_userid(10000);
    // rpc方法响应
    fixbug::GetFriendListResponse response;
    // 发起rpc方法调用 同步的rpc方法调用过程 MprpcChannel::callMethod

    stub.GetFriendList(nullptr, &request, &response, nullptr); // RpcChannel -> RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次 rpc 调用完成，读取调用结果
    if (response.result().errcode() == 0)
    {
        // 调用成功
        std::cout << "rpc GetFriendList response success: " << std::endl;
        int size = response.friends_size();
        for (int i = 0; i < size; i++)
        {
            std::cout << "index: " << i + 1 << "name: " << response.friends(i) << std::endl;
        }
    }
    else
    {
        std::cout << "rpc GetFriendList response error: " << response.result().errmsg() << std::endl;
    }

    return 0;
}