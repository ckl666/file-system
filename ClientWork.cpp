// File Name: ClientWork.cpp
// Author: xiaxiaosheng
// Created Time: Tue 28 May 2019 05:19:38 AM PDT

#include "ClientWork.h"

#define ERR 1
#define EXI 2
#define NOT 3
#define OK  4



int ClientWork::handleMessage(const TcpConnectionPtr &conn, const string &message)
{
    if(strncmp(message.c_str(), "err", 3) == 0)
    {
        return ERR;
    }
    else if(strncmp(message.c_str(), "not", 3) == 0)
    {
        return NOT;
    }
    else if(strncmp(message.c_str(), "exi", 3) == 0)
    {
        return EXI;
    }
    else if(strncmp(message.c_str(), "ok", 2) == 0)
    {
        return OK;
    }
}


void ClientWork::getMessage(const string &src)
{
    if(strncmp(src.c_str(),"ls",2) == 0
        ||strncmp(src.c_str(), "rm", 2) == 0)
    {
        cmdcallback_(src);
    }
    else if(strncmp(src.c_str(), "up", 2) == 0)
    {
        upcallback_(src);
    }
    else if(strncmp(src.c_str(), "down", 4) == 0)
    {
        downcallback_(src);
    }
}
