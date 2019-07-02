#pragma once

#include <string>
#include <iostream>
#include <string.h>
#include <functional>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;

#define OK 1
#define ERR 2


class ServerWork
{
public:
	typedef std::function<void(const TcpConnectionPtr &conn, const string &message)> CallBack;

	ServerWork();

	void setUpFileCallBack(CallBack cb)
	{
		dealUpFile_ = cb;
	}

	void setDownFileCallBack(CallBack cb)
	{
		dealDownFile_ = cb;
	}
	
	void setLsFileCallBack(CallBack cb)
	{
		dealLsFile_ = cb;
	}

	void setRmFileCallBack(CallBack cb)
	{
		dealRmFile_ = cb;
	}

	void setRmFileCliCallBack(CallBack cb)
	{
		dealRmFileCli_ = cb;
	}

	void setUpFileCliCallBack(CallBack cb)
	{
		dealUpFileCli_ = cb;
	}

    void setSynFileCallBack(CallBack cb)
    {
        dealSynFile_ = cb;
    }

	void handleMessage(const TcpConnectionPtr &conn,
						const string &message);

    /*
	void handleAffirm(const TcpConnectionPtr &conn,
						const string &message);
    */

	void handleMessageCli(const TcpConnectionPtr &conn,
						const string &message);
private:
	CallBack dealUpFile_;
	CallBack dealDownFile_;
	CallBack dealRmFile_;
	CallBack dealLsFile_;
	CallBack dealUpFileCli_;
	CallBack dealRmFileCli_;
	CallBack dealSynFile_;
};
