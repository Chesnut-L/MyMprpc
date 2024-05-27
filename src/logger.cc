#include "logger.h"
#include <iostream>
#include <time.h>

// 获取日志单例
Logger &Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger(/* args */)
{
    // 启动专门的写日志线程
    std::thread writeLogTask([&]()
                             {
        for(;;){
            // 获取当天日期 然后取日志信息 写入相应的日志文件中

            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char filename[128] = {0};
            sprintf(filename, "%d-%d-%d-log.txt", nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday);
            // 打开文件
            FILE *pf = fopen(filename, "a+");
            if (pf == nullptr){
                std::cout << "logger file:" << filename << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }
            // 从队列中取出数据
            std::string msg = m_lckQue.Pop();

            // 添加时分秒
            char time_buf[128] = {0};
            sprintf(time_buf, "%d:%d:%d => [%s]", nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec, (m_loglevel == INFO ? "info" : "error"));
            msg.insert(0, time_buf);
            msg.append("\n");

            // 写入文件
            fputs(msg.c_str(), pf);
            // 关闭文件
            fclose(pf);

        } });
    // 设置线程分离，守护线程
    writeLogTask.detach();
}

// 设置日志级别
void Logger::SetLogLevel(LogLevel level)
{
    m_loglevel = level;
}

// 写日志，将日志信息写入lockqueue写入缓冲区
void Logger::Log(std::string msg)
{
    m_lckQue.Push(msg);
}
