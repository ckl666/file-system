#pragma once

//#include "mysql.h"
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

class BalanceWork
{
public:
	//set callback to deal client and server
	typedef std::function<void(const TcpConnectionPtr &,const string &)> Callback;
	BalanceWork(Callback cbs, Callback cdc);

	void handleMessage(const TcpConnectionPtr &conn,const string &message);
private:
	Callback clientCallBack_;
	Callback serverCallBack_;
};
