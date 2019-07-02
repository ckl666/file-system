#include "codec.h"
#include "AnalysisMessage.h"
#include "ClientWork.h"

#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpClient.h>

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <functional>

using namespace std;
using namespace muduo;
using namespace muduo::net;

#define ERR 1
#define EXI 2
#define NOT 3
#define OK  4

#define DOWN_AFFIRM_STATUS 9
#define UP_AFFIRM_STATUS 5
#define RECVING_STATUS 6
#define SENDING_STATUS 7
#define CMD_STATUS 8
class Client : noncopyable
{
public:
    Client(EventLoop* loop, const InetAddress& serverAddr)
        : client_(loop, serverAddr, "Client"),
        codec_(std::bind(&Client::onMessageBal, this, _1, _2, _3)),
        status_(CMD_STATUS),
        fp_(nullptr)
    {
        client_.setConnectionCallback(
            std::bind(&Client::onConnectionBal, this, _1));
        client_.setMessageCallback(
            std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
        client_.enableRetry();
        work_.setUpCallBack(std::bind(&Client::handleUpFile, this, _1));
        work_.setDownCallBack(std::bind(&Client::handleDownFile, this, _1));
        work_.setCmdCallBack(std::bind(&Client::handleCmd, this, _1));
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }


    void handleUpFile(const string &message);

    void handleDownFile(const string &message);

    void handleCmd(const string &message);

    void sendFile();

    void recvFile(const string &);
    
    void write(const string &message);

private:
    
    void getFileHash(const string &filename, string &hash);

    void onMessage(const TcpConnectionPtr &conn,const string &message,Timestamp time);

    void onMessageBal(const TcpConnectionPtr &conn,const string &message,Timestamp time);

    void onConnectionBal(const TcpConnectionPtr &conn);

    void onConnection(const TcpConnectionPtr &conn);
    
    void handleUpAffirm(const TcpConnectionPtr &, const string &);

    void handleDownAffirm(const TcpConnectionPtr &,const string &);



    FILE *fp_;
    int filesize_;
    int currentsize_;
    int status_;
    TcpClient client_;
    TcpClient *clientser_;
    ClientWork work_;
    LengthHeaderCodec codec_;
    TcpConnectionPtr connection_;
};
