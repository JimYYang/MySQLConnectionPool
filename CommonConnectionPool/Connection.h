#pragma once
#include <string>
#include <mysql.h>
#include <ctime>

using namespace std;
/*
* 实现Connection数据库的操作
*/

class Connection
{
public:
	// 初始化数据库连接
	Connection();
	// 释放数据库连接资源
	~Connection();
	// 连接数据库
	bool connect(string ip, unsigned short port, string user, string password,
		string dbname);
	// 更新操作 insert、delete、update
	bool update(string sql);
	// 查询操作 select
	MYSQL_RES* query(string sql);

	// 刷新一下连接的起始空闲时间点
	void refreshAliveTime() { _aliveTime = clock(); }
	// 返回存活的时间
	clock_t getAliveTime()const { return clock() - _aliveTime; }
private:
	MYSQL* _conn; // 表示和Connection SERVER的一条连接
	clock_t _aliveTime; // 记录进入空闲状态后的起始存活时间
};
