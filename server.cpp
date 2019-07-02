#include "server.h"
#include <muduo/net/EventLoopThreadPool.h>


#define RECV_STATUS 1
#define CMD_STATUS 2
#define WAIT_AFFIRM 3
#define SEND_STATUS 4
//#define NEW_SERVER 5

#define PATH "./file/"


const int SYN = 1;


//serversend_;


char *PORT = nullptr;
char *PORTS = nullptr;
const int MAX_LENGTH = 4096;

void onHighWaterMaskCallBack(const TcpConnectionPtr &conn, size_t len)
{
	LOG_INFO << "onHighWaterMask " << len;
}

Server::Server(EventLoop *loop,const InetAddress &listenAddr, const InetAddress &listenAddrsend,
			 const  InetAddress &connectAddr)
	:server_(loop, listenAddr, "Server"),
	serversend_(server_.threadPool()->getNextLoop(), listenAddrsend, "ServerSend"),
	client_(loop, connectAddr, "Client"),
	loop_(loop),
	balanceconn_(nullptr),
	// one's to deal server'message another to dealwith client;
	codec_(std::bind(&Server::onMessage, this, _1, _2, _3)),
	codeccli_(std::bind(&Server::onMessageCli, this, _1, _2, _3))
{
	server_.setConnectionCallback(std::bind(&Server::onConnection,this,_1));
	server_.setMessageCallback( std::bind(&LengthHeaderCodec::onMessage,&codec_ ,_1, _2, _3) );
	server_.setWriteCompleteCallback(std::bind(&Server::sendFile,this,_1));	
	
    serversend_.setConnectionCallback(std::bind(&Server::onConnectionSer, this, _1));
    serversend_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codeccli_, _1, _2, _3));
	
	client_.setConnectionCallback( std::bind(&Server::onConnectionCli, this, _1) );
	client_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codeccli_, _1, _2, _3));

	work_.setUpFileCallBack(std::bind(&Server::handleUpFile,this, _1, _2));
	work_.setDownFileCallBack(std::bind(&Server::handleDownFile,this, _1, _2));
	work_.setLsFileCallBack(std::bind(&Server::handleLsFile,this, _1,_2));
	work_.setUpFileCliCallBack(std::bind(&Server::handleUpFileCli,this, _1, _2));
	work_.setRmFileCallBack(std::bind(&Server::handleRmFile, this, _1, _2));
	work_.setRmFileCliCallBack(std::bind(&Server::handleRmFileCli, this, _1, _2));
    work_.setSynFileCallBack(std::bind(&Server::handleSynFile,this,_1, _2));
}



void Server::handleSynFile(const TcpConnectionPtr &conn, const string &message)
{

    /*
    LOG_INFO << conn->localAddress().toPort() << "->"
             << conn->peerAddress().toPort();
    codec_.send(get_pointer(conn), "shoaasdas");
    */
    MySql my;
    if(!my.initMySql("127.0.0.1", "root", "1234...", "test"))
    {
        LOG_INFO << "mysql connect faild";
        return ;
    }
    
    
    string sql = "select hash from hashtable";
    vector<string> query_res;
    my.query(sql);
    my.resoult(query_res);
    int size = query_res.size();

    coroutine<std::tuple<const TcpConnectionPtr ,FILE **>>::push_type sink{ std::bind(&Server::sendToServer, this, placeholders::_1) };
    for(int i = 0; i < size; i++)
    {
        string filepath = string(PATH) + query_res[i];
        FILE *fp = fopen(filepath.c_str(), "rb");
        if(fp == nullptr)
        {
            continue;
        }
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char buf[20] = {0};
        sprintf(buf, " %d", len);
        string mes = "up a " + query_res[i] + string(buf);
        codec_.send(get_pointer(conn), mes);

        while(fp != nullptr)
        {
            //sink(fp); 
            sink(std::make_tuple(conn, &fp));
        }
    }
}


//
void Server::sendToServer(coroutine<std::tuple<const TcpConnectionPtr, FILE **>>::pull_type &source)
{
    auto val = source.get();
    FILE **fp = get<1>(val);
    const TcpConnectionPtr &conn = get<0>(val);
    while(*fp != nullptr)
    {
        char buf[MAX_LENGTH] = {0};
        if(::fread(buf, 1, MAX_LENGTH, *fp) > 0)
        {
            codec_.send(get_pointer(conn),buf); 
        }
        else
        {
            fclose(*fp);
            *fp = nullptr;
            source();

            auto val = source.get();
            fp = get<1>(val);
        }
    }
}

void Server::handleRmFile(const TcpConnectionPtr &conn, const string &message)
{
    LOG_INFO << message;
    char buff[64] = {0};
    strcpy(buff,message.c_str());
    AnalysisMessage amsg(buff, " ");
    vector<char *> res;
    amsg.spliteMessage(res);
    if(res.size() < 2)
    {
        LOG_INFO << "message info";
        return ;
    }
    FileInfo *fileinfo = &connections_[conn];
    MySql * mysql_ = fileinfo->mysql_;
    string sql = string("select hash from filetable where file_name = '") + string(res[1]) + string("';");
    mysql_->query(sql);
    vector<string> query_res;
    mysql_->resoult(query_res);

    if(query_res.size() > 0)
    {
        string sql = "select count from hashtable where hash = '" + query_res[0] + "';";
        mysql_->query(sql);
        vector<string> tmp_res;
        mysql_->resoult(tmp_res);
        int count = atoi(tmp_res[0].c_str());
        if(count > 1)
        {
            sql.clear();
            sql = string("update hashtable set count = count-1 where hash = '") + query_res[0] + string("';");
            mysql_->query(sql);
            sql.clear();
            sql = string("delete from filetable where file_name = '") + string(res[1]) + string("';");
            mysql_->query(sql);
            codec_.send(get_pointer(conn),"ok");
        }
        else
        {
            sql.clear();
            sql = string("delete from filetable where file_name = '") + string(res[1]) + string("';");
            mysql_->query(sql);

            sql.clear();
            sql = string("delete from hashtable where hash = '") + query_res[0] + string("';");
            mysql_->query(sql);

            string filepath = string(PATH) + query_res[0];
            remove(filepath.c_str());
            codec_.send(get_pointer(conn), "ok");
            string mess = "rm " + query_res[0];
            for(auto it:connectSend_)
            {
                codeccli_.send(get_pointer(it.first), mess);
            }
        }
    }
    else
    {
        LOG_INFO << "rm file not find";
        codec_.send(get_pointer(conn), "err");
        return ;
    }
}

void Server::handleRmFileCli(const TcpConnectionPtr &conn, const string& message)
{
    char buff[64] = {0};
    strcpy(buff, message.c_str());
    AnalysisMessage amsg(buff, " ");
    vector<char *> res;
    amsg.spliteMessage(res);
    if(res.size() < 2)
    {
        LOG_INFO << " error " << message;
    }
    string filepath = string(PATH) + string(res[1]);
    remove(filepath.c_str());
}

//deal multi-threading security
void Server::start()
{
	/*
	if(!mysql_.initMySql("127.0.0.1", "root", "1234...", "test"))
	{
		LOG_INFO << "mysql connect error";
	}
	mysql_.query("set names utf-8");
	*/
	//connect to balance
	client_.connect();
	//server start();
    serversend_.start();
	server_.start();
}

//deal client connect
void Server::onConnection(const TcpConnectionPtr &conn)
{
	LOG_INFO << conn->localAddress().toIpPort() << " -> "
			 << conn->peerAddress().toIpPort() << " is "
			 << (conn->connected()? " UP " : " DOWN ");
	MutexLockGuard lock(mutex_);
	if(conn->connected())
	{
		FileInfo fileinfo;
		fileinfo.fp = nullptr;
		fileinfo.currentSize = 0;
		fileinfo.fileSize = 0;
        fileinfo.filename = nullptr;
		fileinfo.status = CMD_STATUS;
		
		MySql *mysql_ = new MySql();
		
		if(!mysql_->initMySql("127.0.0.1", "root", "1234...", "test"))
		{
			LOG_INFO << "mysql connect error";
			delete mysql_;
			fileinfo.mysql_ = nullptr;
		}
		else
		{
			mysql_->query("set names utf-8");
			fileinfo.mysql_ = mysql_;
		}
		connections_.insert({conn, fileinfo});
        //conn->setWriteCompleteCallback(std::bind(&Server::sendFile, this, _1));
	}
	else
	{
        FileInfo *fileinfo = &connections_[conn];
        if(fileinfo->fp != nullptr)
        {
            fclose(fileinfo->fp);
            fileinfo->fp = nullptr;
            if(fileinfo->currentSize < fileinfo->fileSize)
            {
                string sql = "update filetable set flag = false where file_name = '"
                            + string(fileinfo->filename) + "';";
                free(fileinfo->filename);
                fileinfo->filename = nullptr;
            }

            delete (fileinfo->mysql_);
        }
		connections_.erase(conn);
	}
}


void Server::handleAffirm(const TcpConnectionPtr &conn, const string &message)
{
    if(strncmp(message.c_str(), "ok", 2) == 0)
    {
        connections_[conn].status = SEND_STATUS;
        sendFile(conn);
    }
    else if(strncmp(message.c_str(), "err", 3) == 0)
    {
        FileInfo *fileinfo = &connections_[conn];
        fclose(fileinfo->fp);
        fileinfo->fp = nullptr;
        fileinfo->currentSize = 0;
        fileinfo->fileSize = 0;
        fileinfo->status = CMD_STATUS;
    }
    else
    {
        connections_[conn].status = CMD_STATUS;
    }
}

//deal balance server connect
//connectList connectServers_;
void Server::onConnectionCli(const TcpConnectionPtr &conn)
{
	//onConnection(conn);
	LOG_INFO << conn->name() << "->"
			 << conn->localAddress().toIpPort() << " -> "
			 << conn->peerAddress().toIpPort() << " is "
			 << (conn->connected()? " UP " : " DOWN ");
	if(conn->connected())
	{
		FileInfo fileinfo;
		fileinfo.fp = nullptr;
		fileinfo.currentSize = 0;
		fileinfo.fileSize = 0;
		fileinfo.status = CMD_STATUS;
		if(balanceconn_ != nullptr)
		{
			connectServers_.insert({conn,fileinfo});
		}
		else
		{
			balanceconn_ = conn;
			string str = string("ser 127.0.0.1 ") + string(PORT) + string(" ") + string(PORTS);
			codeccli_.send(get_pointer(conn), str);
		}
	}
	else
	{
		connectServers_.erase(conn);
	}
}

//deal client message
void Server::onMessage(const TcpConnectionPtr &conn,
						const string &message,
						Timestamp time)
{
    LOG_INFO << message;
	int status = connections_[conn].status;
	switch(status)
	{
		case CMD_STATUS:
			work_.handleMessage(conn, message);
			break;
		case RECV_STATUS:
			recvFile(conn,message,true);	
			break;
		case WAIT_AFFIRM:
            handleAffirm(conn, message);
			break;
		default:
			break;
	}
}
//connect server
//connectSend_

void Server::onConnectionSer(const TcpConnectionPtr &conn)
{
	LOG_INFO << conn->localAddress().toIpPort() << " -> "
			 << conn->peerAddress().toIpPort() << " is "
			 << (conn->connected()? " UP " : " DOWN ");
    if(conn->connected())
    {
		FileInfo fileinfo;
		fileinfo.fp = nullptr;
		fileinfo.currentSize = 0;
		fileinfo.fileSize = 0;
		fileinfo.status = CMD_STATUS;
        connectSend_[conn] = fileinfo;
    }
    else
    {
        connectSend_.erase(conn);
    }
}

void Server::connectToServer(const string &message)
{
	char buff[128] = {0};
	strcpy(buff, message.c_str());
	AnalysisMessage amsg(buff, " ");
	vector<char *> res;
	amsg.spliteMessage(res);

	InetAddress connectAddr(res[0],atoi(res[2]));
	TcpClient *client = new TcpClient(loop_, connectAddr,"Ser");
	client->setConnectionCallback(std::bind(&Server::onConnectionCli, this, _1));
	client->setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codeccli_, _1, _2, _3));
	client->connect();
	clients_.insert(client);
}



//deal balance message
//recv from balance's message
void Server::onMessageCli(const TcpConnectionPtr &conn,
							const string &message,
							Timestamp time)
{
	//if balance
    LOG_INFO << message;
	if(conn == balanceconn_)
	{
        if(strncmp(message.c_str(), "syn", 3) == 0)
        {
            auto it = connectSend_.begin();
            if(it == connectSend_.end())
            {
                return ;
            }
            if(SYN == 1)
            {
                codec_.send(get_pointer(it->first),message);
            }
        }
        else
        {
	        connectToServer(message);	
        }
	}
    else
    {
        auto it = connectServers_.find(conn);
        if(it == connectServers_.end())
        {
            it = connectSend_.find(conn);
            if(it == connectSend_.end())
            {
                return ;
            }
        }
        int status = it->second.status;
        
        switch(status)
        {
            case CMD_STATUS:
                work_.handleMessageCli(conn, message);
                break;
            case RECV_STATUS:
                recvFile(conn,message,false);	
                break;
            case WAIT_AFFIRM:
                handleAffirm(conn,message);
                break;
            default:
                break;
        }
    }
}

void Server::sendFile(const TcpConnectionPtr &conn)
{
    /*
    auto it = connecSend_.find(conn);
    if(it != connectSend_.end())
    {
        sendToServer()
    }
    */
	char buff[MAX_LENGTH] = {0};
	FileInfo *fileinfo = &connections_[conn];
    if(fileinfo->status != SEND_STATUS)
    {
        return ;
    }
	int read_bitys = 0;
	if(fileinfo->fp != nullptr)
	{
		if(::fread(buff, 1, MAX_LENGTH, fileinfo->fp) > 0)
		{
			codec_.send(get_pointer(conn),string(buff));
		}
		else
		{
            //this is bug
			::fclose(fileinfo->fp);
			fileinfo->fp = nullptr;
            fileinfo->status = CMD_STATUS;
            if(fileinfo->filename != nullptr)
            {
                free(fileinfo->filename);
                fileinfo->filename = nullptr;
            }
            //conn->shutdown();
		}
	}
	else
	{
		LOG_INFO << "fp is nullptr";
        fileinfo->status = CMD_STATUS;
	}
}

void Server::recvFile(const TcpConnectionPtr &conn, const string &message, bool flag)
{
	int size = message.size();
	//const char *str = message.c_str();
	FileInfo *fileinfo;
	if(flag)
	{
		fileinfo = &connections_[conn];
	}
	else
	{
		fileinfo = &connectServers_[conn];
	}
	fwrite(message.c_str(), size, 1, fileinfo->fp);
	fileinfo->currentSize += size;
	if(fileinfo->currentSize >= fileinfo->fileSize)
	{
		fclose(fileinfo->fp);
		fileinfo->fp = nullptr;
		fileinfo->currentSize = 0;
		fileinfo->fileSize = 0;
		fileinfo->status = CMD_STATUS;
        if(fileinfo->filename != nullptr)
        {
            free(fileinfo->filename);
            fileinfo->filename = nullptr;
        }
	}
	if(flag)
	{
		for(auto it:connectSend_)
		{
			codec_.send(get_pointer(it.first), message);
		}
	}
}


void Server::handleUpFileCli(const TcpConnectionPtr &conn,const string &message)
{
	LOG_INFO << "file up from server";
	char buff[128] = {0};
	strcpy(buff, message.c_str());
	AnalysisMessage amsg(buff, " ");
	vector<char *> res;
	amsg.spliteMessage(res);
	
	string filepath = string(PATH) + string(res[2]);
	FILE *fp = fopen(filepath.c_str(),"ab");
	if(fp == nullptr)
	{
		codec_.send(get_pointer(conn), "err");
		LOG_INFO << "open file faild";
		return ;
	}
	FileInfo *fileinfo;
	fileinfo = &connectServers_[conn];
	
	fileinfo->fp = fp;
	fileinfo->currentSize = 0;
	int size = atoi(res[3]);
	fileinfo->fileSize = size;
	fileinfo->status = RECV_STATUS;
}

bool Server::judgeFileFlag(MySql *mysql_, char *filename, char *hash)
{
    vector<string> query_res;
    string sql = "select hash,flag from filetable where file_name = '"
                    + string(filename)
                    + string ("';");
    mysql_->query(sql);
    mysql_->resoult(query_res);
    vector<char *> res;
    char buff[64] = {0};
    strcpy(buff, query_res[0].c_str());
    AnalysisMessage amsg(buff, " ");
    
    amsg.spliteMessage(res);
    if(res.size() < 2)
    {
        LOG_INFO << "error";
        return false;
    }
    if(strcmp(res[0],hash) != 0)
    {
        LOG_INFO << "filename is exited";
        return false;
    }
    
    int flag = atoi(res[1]);
    if(flag == 1)
    {
        LOG_INFO << "file is exited";
        return false;
    }
    return true;
}



void Server::handleUpFile(const TcpConnectionPtr &conn,const string &message)
{
	LOG_INFO << " up file ";
	char buff[128] = {0};
	strcpy(buff, message.c_str());
	AnalysisMessage amsg(buff, " ");
	vector<char *> res;
	amsg.spliteMessage(res);
	MySql *mysql_ = connections_[conn].mysql_;
	if(mysql_ == nullptr)
	{
		LOG_INFO << "error mysql";
		codec_.send(get_pointer(conn),"err");
		return ;
	}
	if(res.size() < 4)
	{
		LOG_INFO << "error";
		codec_.send(get_pointer(conn), "err");
		return ;
	}
    bool flag = false;
	if(judgeFileName(mysql_, res[1]))
	{
        if(!judgeFileFlag(mysql_, res[1], res[2]))
        {
            return ;
        }
        flag = true;
	}
	if(!flag && judgeHashValue(mysql_, res[2]))
	{
		LOG_INFO << "file is exited";
		string sql = string("insert into filetable(file_name, hash) values('") + string(res[1]) + string("','") + string(res[2]) + string("');");
		mysql_->query(sql);
		sql.clear();
		sql = string("update hashtable set count=count+1 where hash='")+ string(res[2]) + string("';");
		mysql_->query(sql);
		codec_.send(get_pointer(conn), "exi");
		//update database;
		return ;
	}
	else
	{
		string filepath = string(PATH) + string(res[2]);
		FILE *fp = fopen(filepath.c_str(),"ab");
		if(fp == nullptr)
		{
			codec_.send(get_pointer(conn), "err");
			LOG_INFO << "open file faild";
			return ;
		}
        int offset = ftell(fp);
		FileInfo *fileinfo =  &connections_[conn];
		string sql = string("insert into filetable(file_name, hash) values('") + string(res[1]) + string("','") + string(res[2]) + string("');");
		mysql_->query(sql);
		sql.clear();
		sql = string("insert into hashtable(hash,count) values('")+string(res[2]) + string("',1);");
		mysql_->query(sql);

		fileinfo->fp = fp;
		fileinfo->currentSize = 0;
		int size = atoi(res[3]);
		fileinfo->fileSize = size;
		fileinfo->status = RECV_STATUS;
        char *filename = (char *)malloc(sizeof(char)*strlen(res[1]) + 1);
        strcpy(filename, res[1]);
        fileinfo->filename = filename;

        char off_buf[20] = {0};
        sprintf(off_buf, "%d", offset);
        string mess = string("ok") + string(off_buf);
        codec_.send(get_pointer(conn), mess);
		
        for(auto val:connectSend_)
		{
			codeccli_.send(get_pointer(val.first), message);
		}
	}
}

bool Server::judgeFileName(MySql *mysql_, char *filename)
{
	vector<string> query_res;
	string sql = string("select * from filetable where file_name = '") + string(filename) + string("';");
	mysql_->query(sql);
	mysql_->resoult(query_res);
	if(query_res.size() != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Server::judgeHashValue(MySql *mysql_, char *hashvalue)
{
	vector<string> query_res;
	string sql = string("select * from hashtable where hash = '") + string(hashvalue) + string("';");
	mysql_->query(sql);
	mysql_->resoult(query_res);
	if(query_res.size() != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Server::handleDownFile(const TcpConnectionPtr &conn, const string &message)
{
	LOG_INFO << " down file ";	
	char buff[128] = {0};
	strcpy(buff, message.c_str());
	AnalysisMessage amsg(buff, " ");
	vector<char *> res;
	amsg.spliteMessage(res);

	MySql *mysql_ = connections_[conn].mysql_;
	if(mysql_ == nullptr)
	{
		LOG_INFO << "error mysql";
		codec_.send(get_pointer(conn), "err");
		return ;
	}
	if(res.size() < 2)
	{
		LOG_INFO << "error";
		codec_.send(get_pointer(conn),"err");
		return ;
	}
	if(!judgeFileName(mysql_, res[1]))
	{
		LOG_INFO << "file not exited";
		codec_.send(get_pointer(conn), "not");
		return ;
	}
	else 
	{
		string sql = string("select hash from filetable where file_name = '") 
								+ string(res[1]) + string("';");
		std::vector<string>	query_res;
		mysql_->query(sql);
		mysql_->resoult(query_res);
		string filepath = string(PATH) + query_res[0];
		FILE *fp = fopen(filepath.c_str(), "rb");
		if(fp == nullptr)
		{
			LOG_INFO << "file open faild";
			codec_.send(get_pointer(conn),"err");
			return ;
		}
		FileInfo *fileinfo = &connections_[conn];
		fseek(fp, 0, SEEK_END);
		unsigned int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char buff[15] = {0};
		sprintf(buff, "ok%d", size);

        fileinfo->status = WAIT_AFFIRM;
		fileinfo->fp = fp;
		codec_.send(get_pointer(conn), buff);
	}
}

void Server::handleLsFile(const TcpConnectionPtr &conn,const string &message)
{

	MySql *mysql_ = connections_[conn].mysql_;
	if(mysql_ == nullptr)
	{
		LOG_INFO << "error mysql";
		codec_.send(get_pointer(conn), "err");
		return ;
	}
	string sql = "select file_name from filetable";
	std::vector<string>	query_res;
	mysql_->query(sql);
	mysql_->resoult(query_res);
	for(string val : query_res)
	{
		codec_.send(get_pointer(conn), val);
	}
}



int main(int argc, char *argv[])
{
	if(argc > 1)
	{
		EventLoop loop;
		InetAddress listenAddr(atoi(argv[1]));
		InetAddress listenAddrsend(atoi(argv[2]));
		PORT = argv[1];
        PORTS = argv[2];
		//balance server addr
		InetAddress connectAddr(8888);
		Server server(&loop, listenAddr,listenAddrsend,connectAddr);
		server.setThreadNum(8);
		server.start();
		loop.loop();
	}
	return 0;
}
