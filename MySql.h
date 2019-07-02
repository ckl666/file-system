#pragma once

#include <mysql.h>
#include <string>
#include <iostream>
#include <vector>

using namespace std;


class MySql
{
public:
	MySql():_mysql(nullptr)
	{
		_mysql = mysql_init(nullptr);
	}

    ~MySql()
    {
        mysql_close(_mysql); 
    }
	
	bool initMySql( const string &IP, const string &name, 
		const string &pwd,const string &database);

	void query(const string sql);

	void resoult(std::vector<string> &query_res);
private:
	MYSQL *_mysql;
};
