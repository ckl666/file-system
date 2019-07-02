#pragma once

#include "AnalysisMessage.h"

#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>

#include <functional>
#include <string>
#include <string.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;

class ClientWork
{
public:
    
    typedef std::function<void(const std::string &message)> sendCallBack;
    //typedef std::function<void(const TcpConnectionPtr &conn, const string &message)> recvCallBack;
    
    int handleMessage(const TcpConnectionPtr &conn,const string &message);


    void setCmdCallBack(sendCallBack cb)
    {
        cmdcallback_ = cb;
    }

    void setUpCallBack(sendCallBack cb)
    {
        upcallback_ = cb;
    }

    void setDownCallBack(sendCallBack cb)
    {
        downcallback_ = cb;
    }

    void getMessage(const string &src);

private:

    sendCallBack cmdcallback_;
    sendCallBack upcallback_;
    sendCallBack downcallback_; 
};
