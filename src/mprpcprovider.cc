#include "mprpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include <iostream>
#include "logger.h"
#include "zookeeperutil.h"
/*
    service_name -> service描述
                                -> service* 记录服务对象
                                -> method_name -> method方法对象
*/
// 框架提供给外部使用的，用于发布rpc方法的接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    // 获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务名称
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service的方法数量
    int methodCnt = pserviceDesc->method_count();

    // std::cout << "service_name:" << service_name << std::endl;
    LOG_INFO("service_name:%s", service_name.c_str());

    ServiceInfo service_info;
    for (int i = 0; i < methodCnt; i++)
    {
        // 获取服务对象指定下标的服务方法的描述 （抽象描述）
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        // std::cout << "method_name:" << method_name << std::endl;
        LOG_INFO("method_name:%s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
    // 组合了TcpServer
    std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr;
    // 获取配置
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    // 绑定连接回调和消息读写回调方法 分离网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置muduo库线程数量
    server.setThreadNum(4);

    // ZooKeeper --------------------------------------------------------------------------------
    // 将当前rpc节点上要发布的服务都注册到zk上， 让rpc client可以从zk上发现服务
    // session timeout 30s zkclient API网络I/O线程 1/3 * timeout时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();
    // service_name 为永久性节点 method_name为临时性节点
    for (auto &sp : m_serviceMap){
        // /service_name
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        // method_name
        for (auto &mp : sp.second.m_methodMap){
            // /service_name/method_name
            std::string method_path = service_path + "/" + mp.first;
            // 存储当前这个rpc服务节点的主机和端口
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }
    // -------------------------------------------------------------------------------------------

    // std::cout << "RpcProvider start service at ip :" << ip << ", port:" << port << std::endl;
    LOG_INFO("RpcProvider start service at ip :%s port: %d", ip.c_str(), port);

    // 启动网络服务
    server.start();

    // 启动epoll_wait 以阻塞方法等待新的连接
    m_eventLoop.loop();
}

// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和 rpc 客户端断开连接
        conn->shutdown();
    }
}

/*
    在框架内部， rpcProvider和rpcConsumer协商好之间通信的protobuf数据类型
    service_name method_name args 定义proto的message类型，定义数据头的序列化和反序列化
                                  数据头： service_name method_name args_size 记录参数长度，截取参数长度字节流作为原始信息，粘包后也能够取出原始数据

    header_size（4个字节） + header_str + args_str

    std::string insert/copy方法
*/
// 已建立连接用户的读写事件回调 如果远程有一个rpc服务的调用请求 OnMessage就会进行响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp)
{
    // 网络上接收的远程rpc调用请求的字符流  方法名字 参数
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前四个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char *)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 数据头反序列化失败
        // std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        LOG_ERR("rpc_header_str:%s  parse error!", rpc_header_str.c_str());
        return;
    }

    // 获取rpc方法参数原始数据流
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 打印调试信息
    // std::cout << "============================================================" << std::endl;
    // std::cout << "  rpcprovider  " << std::endl;
    // std::cout << "header_size:" << header_size << std::endl;
    // std::cout << "rpc_header_str:" << rpc_header_str << std::endl;
    // std::cout << "service_name:" << service_name << std::endl;
    // std::cout << "method_name:" << method_name << std::endl;
    // std::cout << "args_str:" << args_str << std::endl;
    // std::cout << "=============================================================" << std::endl;
    LOG_INFO("=============================================================");
    LOG_INFO("RPCProvider:");
    LOG_INFO("header_size:%d",header_size);
    LOG_INFO("rpc_header_str:%s",rpc_header_str.c_str());
    LOG_INFO("args_str:%s",args_str.c_str());
    LOG_INFO("=============================================================");

    // 获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        // std::cout << service_name << " is not exist!" << std::endl;
        LOG_ERR("%s is not exist!", service_name.c_str());
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        // std::cout << service_name << " : " << method_name << " is not exist!" << std::endl;
        LOG_ERR("%s is not exist!", method_name.c_str());
        return;
    }

    google::protobuf::Service *service = it->second.m_service;      // 获取service对象
    const google::protobuf::MethodDescriptor *method = mit->second; // 获取method对象

    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        // std::cout << "request parse error!" << std::endl;
        LOG_ERR("request parse error!");
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    // inline Closure* NewCallback(Class* object, void (Class::*method)(Arg1, Arg2), Arg1 arg1, Arg2 arg2)
    // <>内强制指定模板数据类型
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider, const muduo::net::TcpConnectionPtr &, google::protobuf::Message *>(this, &RpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    // new UserService().Login(controller,request,response,done)
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    // 对相应序列化
    if (response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络将rpc方法执行结果发送回rpc调用方
        conn->send(response_str);
    }
    else
    {
        // std::cout << "Serialize response_str error!" << std::endl;
        LOG_ERR("Serialize response_str error!");
    }
    // 模拟http短连接服务，由rpcProvider主动断开连接
    conn->shutdown();
}