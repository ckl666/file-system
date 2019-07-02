#include "codec.h"
#include "balanceWork.h"
#include "AnalysisMessage.h"

#include <muduo/base/Mutex.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>


#include <map>
#include <set>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;

class BalanceServer : noncopyable
{
public:
	BalanceServer(EventLoop *loop, const InetAddress &listenAddr);

	//select server and send to client ip and port;
	void dealClientRequest(const TcpConnectionPtr &conn,const string &message);
	
	//add server's ip and port to connections_;
	void dealServerRequest(const TcpConnectionPtr &conn,const string &message);

	//get a server addr
	string &getNextServer();

	void transmitMessage(const TcpConnectionPtr &conn, const string &message, Timestamp time);

	void start();
private:
	void onConnection(const TcpConnectionPtr &conn);

	void onMessage(const TcpConnectionPtr &conn, const string &massage,Timestamp time);
	
	TcpConnectionPtr getNextConn();

	typedef std::set<TcpConnectionPtr> ConnectList;
	typedef std::map<TcpConnectionPtr, std::string> ConnectMap;
	//ConnectList connections_ GUARDED_BY(mutex_);
	TcpServer server_;
	MutexLock mutex_;
	ConnectMap serconnections_ GUARDED_BY(mutex_);
	//ConnectMap serconnections_;
	LengthHeaderCodec codec_;
	//save server ip and port is active
	BalanceWork work_;
	int next_;
};

BalanceServer::BalanceServer(EventLoop *loop, const InetAddress &listenAddr)
	:next_(0),
	server_(loop,listenAddr,"BanlaceServer"),
	codec_(std::bind(&BalanceServer::onMessage,this, _1, _2, _3)),
	work_(std::bind(&BalanceServer::dealClientRequest,this, _1, _2)
		,std::bind(&BalanceServer::dealServerRequest,this, _1, _2))
{
	server_.setConnectionCallback(
		std::bind(&BalanceServer::onConnection,this,_1));
	server_.setMessageCallback(
		std::bind(&LengthHeaderCodec::onMessage,&codec_,_1,_2,_3));
}

//add ip and port
void BalanceServer::dealServerRequest(const TcpConnectionPtr &conn,const string &message)
{
	LOG_INFO << "deal with erver";
	string addr(message.begin()+4, message.end());
	MutexLockGuard lock(mutex_);
	if(conn->connected())
	{	
		//update other server's connect table;
		for(auto val:serconnections_)
		{
			codec_.send(get_pointer(val.first), addr);
			codec_.send(get_pointer(conn),val.second);
		}
		serconnections_[conn] = addr;
        codec_.send(get_pointer(conn),"syn");
	}
	else
	{
		serconnections_.erase(conn);
	}
}


//deal client request and send to ip and port to client
void BalanceServer::dealClientRequest(const TcpConnectionPtr &conn,const string &message)
{
	string str;
	if(!serconnections_.empty())
	{
		str = getNextServer();
	}
	else
	{
		str = "err";
	}
	codec_.send(get_pointer(conn), str);
	//close connection
	conn->disconnected();

}

string &BalanceServer::getNextServer()
{
	srand(time(NULL));
	int len = serconnections_.size();
	int rad = rand() % len;
	int i = 0;
	auto it = serconnections_.begin();
	while(i < rad)
	{
		i++;
		++it;
	}
	return it->second;
}


void BalanceServer::start()
{
	server_.start();
}

void BalanceServer::onConnection(const TcpConnectionPtr &conn)
{
	LOG_INFO << conn->peerAddress().toIpPort() << " -> "
			 << conn->localAddress().toIpPort() << " is "
			 << (conn->connected()? " UP ": " DOWN ");

    MutexLockGuard lock(mutex_);
    if(conn->connected())
    {
    }
    else
	{
        serconnections_.erase(conn);
	}
}

void BalanceServer::onMessage(const TcpConnectionPtr &conn,const string &message, Timestamp time)
{
	cout << message << endl;
	work_.handleMessage(conn,message);
	//work_ to dealwith message
}

//lock
void BalanceServer::transmitMessage(const TcpConnectionPtr &conn, const string &message, Timestamp time)
{
	auto it = serconnections_.begin();
	if(it->first != conn)
	{
		codec_.send(get_pointer(it->first), message);
	}
}

int main()
{
	EventLoop loop;
	InetAddress listenAddr(8888);
	BalanceServer server(&loop, listenAddr);
	server.start();
	loop.loop();
	return 0;
}
