#pragma once

#include "codec.h"
#include "ServerWork.h"
#include "MySql.h"
#include "AnalysisMessage.h"

#include <muduo/base/Logging.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/TcpClient.h>

#include <boost/coroutine/all.hpp>

#include <tuple>
#include <set>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace boost::coroutines;

typedef struct FileInfo
{
	FILE *fp;
	MySql *mysql_;
	int currentSize;
	int fileSize;
	int status;
	int sign;
    char *filename;
}FileInfo;


class Server:noncopyable
{
public:
	Server(EventLoop *loop, const InetAddress &listenAddr, const InetAddress &,
			const InetAddress &connectAddr);
	
	//start server
	void start();

	void setThreadNum(int numThreads)
	{
		server_.setThreadNum(numThreads);
	}

	void handleUpFile(const TcpConnectionPtr &conn, const string &);

	void handleAffirm(const TcpConnectionPtr &conn, const string &);

	void handleUpFileCli(const TcpConnectionPtr &conn, const string &);

	void handleDownFile(const TcpConnectionPtr &conn, const string &);

	void handleLsFile(const TcpConnectionPtr &conn,const string &);

	void handleOk(const TcpConnectionPtr &conn, const string &message);

	void handleErr(const TcpConnectionPtr &conn, const string &message);

	void sendFile(const TcpConnectionPtr &conn);

    void sendToServer(coroutine<tuple<const TcpConnectionPtr,FILE **>>::pull_type &source);

	void connectToServer(const string &);

	void recvFile(const TcpConnectionPtr &conn, const string &message, bool);

	void handleRmFile(const TcpConnectionPtr &conn, const string &message);

	void handleRmFileCli(const TcpConnectionPtr &conn, const string &message);
    
    void handleSynFile(const TcpConnectionPtr &conn, const string &message);

	bool judgeFileName(MySql *mysql_, char *filename);

	bool judgeHashValue(MySql *mysql_, char *hashvalue);

    bool judgeFileFlag(MySql *mysql_, char *filename, char *hashvalue);

private:
	void onConnection(const TcpConnectionPtr &conn);
	void onConnectionCli(const TcpConnectionPtr &conn);
	void onConnectionSer(const TcpConnectionPtr &conn);


	void onMessage(const TcpConnectionPtr &conn,
				const string &message,
				Timestamp time);

	void onMessageCli(const TcpConnectionPtr &conn,
				const string &message,
				Timestamp time);

	typedef map<TcpConnectionPtr,FileInfo> connectList;
	EventLoop *loop_;
	TcpServer server_;
	TcpServer serversend_;
	ServerWork work_;
	MutexLock mutex_;
	MutexLock mutexcli_;
	LengthHeaderCodec codec_;
	LengthHeaderCodec codeccli_;
	connectList connections_ GUARDED_BY(mutex_);
	connectList connectSend_ ;
	TcpClient client_;
	std::set<TcpClient*> clients_;
	connectList connectServers_ GUARDED_BY(mutexcli_);
	TcpConnectionPtr balanceconn_;
};
