#include "MySql.h"



bool MySql::initMySql(
		const string &IP, 
		const string &name, 
		const string &pwd,
		const string &database)
{
	if(!mysql_real_connect(_mysql, IP.c_str(), name.c_str(),
	pwd.c_str(), database.c_str(), 3306, nullptr, 0))
	{
		cout << "mysql connevt faild" << endl;
		return false;
	}
	return true;
}

void MySql::query(const string sql)
{
	mysql_query(_mysql, sql.c_str());
}

void MySql::resoult(std::vector<string> &query_res)
{
	MYSQL_RES *res;
	res = mysql_store_result(_mysql);
	int count = mysql_field_count(_mysql);
	if(count == 0)
	{
		mysql_free_result(res);
		return ;
	}
	int fields = mysql_num_fields(res);
	MYSQL_ROW row;
	while(row = mysql_fetch_row(res))
	{
		string strtmp;
		for(int i = 0; i < fields; i++)
		{
			strtmp = strtmp + static_cast<string>(row[i]);
			if(i < fields-1)
			{
				strtmp = strtmp + static_cast<string>(" ");
			}
		}
		query_res.push_back(strtmp);
	}
	mysql_free_result(res);
}

/*
int main()
{
	vector<string> query_res;
	MySql mysql;
	if(!mysql.initMySql("127.0.0.1", "root", "1234...", "test"))
	{
		return 0;
	}
	mysql.query("set names gbk");
	mysql.query("select * from stu");
	mysql.resoult(query_res);
	for(auto val:query_res)
	{
		cout << val << endl;
	}
	return 0;
}
*/
