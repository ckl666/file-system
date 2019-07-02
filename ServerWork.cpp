#include "ServerWork.h"

#include <iostream>

using namespace std;
//set server's callback 
ServerWork::ServerWork()
{}

//dealwith server's cmd
void ServerWork::handleMessage(const TcpConnectionPtr &conn,
							const string &message)
{
	if(strncmp(message.c_str(), "up", 2) == 0)
	{
		dealUpFile_(conn,message);
	}
	else if(strncmp(message.c_str(), "down", 4) == 0)
	{
		dealDownFile_(conn,message);
	}
	else if(strncmp(message.c_str(), "ls", 2) == 0)
	{
		dealLsFile_(conn,message);
	}
	else if(strncmp(message.c_str(), "rm", 2) == 0)
	{
		dealRmFile_(conn,message);
	}
}

void ServerWork::handleMessageCli(const TcpConnectionPtr &conn,const string &message)
{
    cout << message << endl;
	if(strncmp(message.c_str(), "up", 2) == 0)
	{
		dealUpFileCli_(conn,message);
	}
	else if(strncmp(message.c_str(), "rm", 2) == 0)
	{
		dealRmFileCli_(conn,message);
	}
    else if(strncmp(message.c_str(), "syn", 3) == 0)
    {
        dealSynFile_(conn, message); 
    }
}



//dealwith server's status
/*
int ServerWork::handleAffirm(const TcpConnectionPtr &conn,
							const string &message)
{
	//status is ok
	if(strncmp(message.c_str(), "ok", 2) == 0)
	{
		return OK;
	}
	//status is error
	else if(strncmp(message.c_str(),"err", 3) == 0)
	{
		return ERR;
	}
}
*/
