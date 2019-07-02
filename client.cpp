// File Name: client.cpp
// Author: xiaxiaosheng
// Created Time: Tue 28 May 2019 12:21:48 AM PDT

#include "client.h"

#define MAX_LENGTH 4096

void Client::onMessageBal(
        const TcpConnectionPtr &conn,
        const string &message,
        Timestamp time)
{
    LOG_INFO << message;

    char buff[64] = {0};
    strcpy(buff, message.c_str());
    AnalysisMessage amsg(buff, " ");
    std::vector<char *> res;
    amsg.spliteMessage(res);
    if(res.size() == 3)
    {
        InetAddress connectAddr(res[0],atoi(res[1]));
        clientser_ = new TcpClient(conn->getLoop(), connectAddr,"client");
        clientser_->setConnectionCallback(std::bind(&Client::onConnection, this, _1));
        clientser_->setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
        codec_.setMessageCallback(std::bind(&Client::onMessage, this, _1, _2, _3));
        clientser_->setWriteCompleteCallback(std::bind(&Client::sendFile, this));
        clientser_->connect();
        //CurrentThread::sleep(1000*1000);
    }
}

void Client::onMessage(
        const TcpConnectionPtr &conn,
        const string &message,
        Timestamp time)
{
    LOG_INFO << message;
    switch(status_)
    {
        case CMD_STATUS:
            break;
        case RECVING_STATUS:
            recvFile(message);
            break;
        case SENDING_STATUS:
            break;
        case UP_AFFIRM_STATUS:
            handleUpAffirm(conn, message);
            break;
        case DOWN_AFFIRM_STATUS:
            handleDownAffirm(conn,message);
            break;
        default:
            break;
    }
}

void Client::handleDownAffirm(const TcpConnectionPtr &conn, const string &message)
{
    int res = work_.handleMessage(conn, message);
    if(res == OK)
    {
        filesize_ = atoi(message.c_str()+2);
        currentsize_ = 0;
        if(filesize_ <= currentsize_)
        {
            fclose(fp_);
            fp_ = nullptr;
        }
        status_ = RECVING_STATUS;
        codec_.send(get_pointer(conn), "ok");
    }
    else if(res == ERR)
    {
        fclose(fp_);
        fp_ = nullptr;
        status_ = CMD_STATUS;
    }
}


void Client::handleUpAffirm(const TcpConnectionPtr &conn, const string &message)
{
    int res = work_.handleMessage(conn, message);
    if(res == OK)
    {
        int offset = atoi(message.c_str()+2);
        fseek(fp_, offset, SEEK_SET);
        status_ = SENDING_STATUS;
        sendFile();
    }
    else if(res == ERR)
    {
        //LOG_INFO << message;
        status_ = CMD_STATUS;
        fclose(fp_);
        fp_ = nullptr;
    } 
}



void Client::recvFile(const string &message)
{
    int size = message.size();
    ::fwrite(message.c_str(), size, 1, fp_);
    currentsize_ += size;
    if(currentsize_ >= filesize_)
    {
        fclose(fp_);
        fp_ = nullptr;
        filesize_ = 0;
        currentsize_ = 0;
        status_ = CMD_STATUS;
    }
}


void Client::sendFile()
{
    if(status_ != SENDING_STATUS)
    {
        return ;
    }
    LOG_INFO << "sendFile";
    if(fp_ == nullptr)
    {
        status_ = CMD_STATUS;
        return;
    }
    char send_buff[MAX_LENGTH] = {0};
    if(::fread(send_buff, 1, MAX_LENGTH, fp_) > 0)
    {
        codec_.send(get_pointer(connection_), string(send_buff));
    }
    else
    {
        fclose(fp_);
        fp_ = nullptr;
        currentsize_ = 0;
        filesize_ = 0;
        status_ = CMD_STATUS;
        //connection_->setWriteCompleteCallback(nullptr);
        //connection_->shutdown();
    }
}

void Client::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected()? " UP " :" DOWN ");
    if(conn->connected())
    {
        client_.disconnect();
        connection_ = conn;
    }
    else
    {
        //connection_.reset();
        client_.connect();
    }
}


void Client::onConnectionBal(const TcpConnectionPtr &conn)
{
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected()? " UP " :" DOWN ");
    if(conn->connected())
    {
        connection_ = conn;
        codec_.send(get_pointer(conn), "cli");
    }
}

void Client::getFileHash(const string &filename, string &hash)
{
    string cmd = string("md5sum ") + filename + string(" > hash.tmp"); 
    system(cmd.c_str());
    char buff[64] = {0};
    FILE *fp = fopen("hash.tmp", "rb");
    if(fp == nullptr) { return ; }
    if(::fread(buff, 1, 64, fp) > 0)
    {
        AnalysisMessage amsg(buff, " ");
        vector<char *> res;
        amsg.spliteMessage(res);
        hash = res[0];
    }
    fclose(fp);
}


void Client::write(const string &message)
{
    work_.getMessage(message);
}

void Client::handleUpFile(const string &message)
{
    char buff[128] = {0};
    strcpy(buff, message.c_str());
    AnalysisMessage amsg(buff, " ");
    vector<char *> res;
    amsg.spliteMessage(res);

    fp_ = fopen(res[1], "rb");
    if(fp_ == nullptr)
    {
        LOG_INFO << "file not find";
        return ;
    }
    fseek(fp_, 0, SEEK_END);
    int size = ftell(fp_);
    fseek(fp_, 0, SEEK_SET);
    
    string hash;
    getFileHash(res[1], hash);
    if(hash.size() == 0)
    {
        ::fclose(fp_);
        fp_ == nullptr;
        return ;
    }
    char size_buf[20] = {0};
    sprintf(size_buf, " %d", size);
    
    string str = string("up ") + string(res[1]) + string (" ") + hash + string(size_buf);

    status_ = UP_AFFIRM_STATUS;
    //LOG_INFO << str;
    codec_.send(get_pointer(connection_), str);
}




void Client::handleDownFile(const string &message)
{
    codec_.send(get_pointer(connection_), message);
    char buff[128] = {0};
    strcpy(buff, message.c_str());
    AnalysisMessage amsg(buff, " ");
    vector<char *> res;
    amsg.spliteMessage(res);
    fp_ = fopen(res[1], "wb");
    if(fp_ == nullptr)
    {
        LOG_INFO << "file open faild";
        return ;
    }
    status_ = DOWN_AFFIRM_STATUS;
}

void Client::handleCmd(const string &message)
{
    codec_.send(get_pointer(connection_), message);
}



int main(int argc,char *argv[])
{

    EventLoopThread loopThread;
    if(argc > 2)
    {
        int port = atoi(argv[2]);        
        InetAddress connectAddr(argv[1], port);
        Client client(loopThread.startLoop(), connectAddr);
        client.connect();
        std::string line;
        while(std::getline(std::cin, line))
        {
            client.write(line);
        }
    }
    return 0;
}

