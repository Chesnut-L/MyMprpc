#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <iostream>

// 全局的watcher观察器 zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
    {
        if (state == ZOO_CONNECTED_STATE)   // zkclient和zkserver连接成功
        {
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);  // 信号量资源+1
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr) {}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        // 关闭句柄 释放资源
        zookeeper_close(m_zhandle);
    }
}

// zkclient启动连接zkserver
void ZkClient::Start()
{
    // 获取配置信息 组织成zk要求格式
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    /* 
        zookeeper_mt : 多线程版本
        zookeeper的API客户端程序提供了三个线程：
            1）API调用线程 当前线程
                ZOOAPI zhandle_t *zookeeper_init(const char *host, watcher_fn fn, int recv_timeout, const clientid_t *clientid, void *context, int flags);
            2）网络IO线程 pthread_create poll
            3）watcher回调线程 pthread_create
    */
    //    创建句柄资源
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (m_zhandle == nullptr)
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 信号量
    sem_t sem;
    sem_init(&sem, 0, 0);
    // 给指定的句柄添加上下文信息（给监听器传递参数）
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);
    std::cout << "zookeeper_init success!" << std::endl;
}

// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 先判断path表示的znode节点是否存在，如果存在就不再重复创建
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (flag == ZNONODE) {
        // path表示的znode节点不存在 创建新节点
        flag = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK){
            std::cout << "znode create success... path: " << path << std::endl;
        }
        else {
            std::cout << "flag:" << flag << std::endl;
            std::cout << "znode create error... path: " << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

// 根据参数指定的路径获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK){
        std::cout << "get znode error... path:" << path << std::endl;
        return "";
    } else {
        return buffer;
    }
}