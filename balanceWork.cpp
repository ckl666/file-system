#include "balanceWork.h"


//set callback function
BalanceWork::BalanceWork(Callback cbc, Callback cbs)
	:clientCallBack_(cbc),
	serverCallBack_(cbs)
{}

void BalanceWork::handleMessage(const TcpConnectionPtr &conn, const string &message)
{
	//splite message
	//client request;
	if(strncmp(message.c_str(), "cli", 3) == 0)
	{
		clientCallBack_(conn,message);
	}
	else if(strncmp(message.c_str(), "ser", 3) == 0)
	{
		//server request
		serverCallBack_(conn,message);
	}
}
