#include "pch.h"
#include <iostream>
#include <iostream>
#include "Connection.h"
#include "CommonConnectionPool.h"

using namespace std;

int main()
{
	// 1. 不使用连接池 单线程 1000条操作
	//clock_t beg = clock();
	//for (int i = 0; i < 1000; i++)
	//{
	//	Connection conn;
	//	char sql[1024] = { 0 };
	//	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	//		"zhang san", 20, "male");
	//	conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
	//	conn.update(sql);
	//}
	//clock_t end = clock();
	//cout << end - beg << endl;

	// 2. 使用连接池 单线程 1000条操作
	//clock_t beg = clock();
	//ConnectionPool *cp = ConnectionPool::getConnectionPool();
	//for (int i = 0; i < 1000; i++)
	//{
	//	char sql[1024] = { 0 };
	//	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	//		"zhang san", 20, "male");
	//	shared_ptr<Connection> sp = cp->getConnection();
	//	sp->update(sql);
	//}

	//clock_t end = clock();
	//cout << end - beg << endl;

	// 3. 使用连接池 4线程 1000条操作
	//clock_t beg = clock();
	//thread t1([]() {
	//	ConnectionPool *cp = ConnectionPool::getConnectionPool();
	//	for (int i = 0; i < 1000; i++)
	//	{
	//		char sql[1024] = { 0 };
	//		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	//			"zhang san", 20, "male");
	//		shared_ptr<Connection> sp = cp->getConnection();
	//		sp->update(sql);
	//	}
	//});

	//thread t2([]() {
	//	ConnectionPool *cp = ConnectionPool::getConnectionPool();
	//	for (int i = 0; i < 1000; i++)
	//	{
	//		char sql[1024] = { 0 };
	//		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	//			"zhang san", 20, "male");
	//		shared_ptr<Connection> sp = cp->getConnection();
	//		sp->update(sql);
	//	}
	//});

	//thread t3([]() {
	//	ConnectionPool *cp = ConnectionPool::getConnectionPool();
	//	for (int i = 0; i < 1000; i++)
	//	{
	//		char sql[1024] = { 0 };
	//		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	//			"zhang san", 20, "male");
	//		shared_ptr<Connection> sp = cp->getConnection();
	//		sp->update(sql);
	//	}
	//});

	//thread t4([]() {
	//	ConnectionPool *cp = ConnectionPool::getConnectionPool();
	//	for (int i = 0; i < 1000; i++)
	//	{
	//		char sql[1024] = { 0 };
	//		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	//			"zhang san", 20, "male");
	//		shared_ptr<Connection> sp = cp->getConnection();
	//		sp->update(sql);
	//	}
	//});

	//t1.join();
	//t2.join();
	//t3.join();
	//t4.join();

	//clock_t end = clock();
	//cout << end - beg << endl;

	// 4. 不使用连接池 4线程 1000条操作
	Connection conn;
	conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
	clock_t beg = clock();
	thread t1([]() {
		for (int i = 0; i < 1000; i++)
		{
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
			conn.update(sql);
		}
	});

	thread t2([]() {
		for (int i = 0; i < 1000; i++)
		{
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
			conn.update(sql);
		}
	});

	thread t3([]() {
		for (int i = 0; i < 1000; i++)
		{
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
			conn.update(sql);
		}
	});

	thread t4([]() {
		for (int i = 0; i < 1000; i++)
		{
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
			conn.update(sql);
		}
	});

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	clock_t end = clock();
	cout << end - beg << endl;
	return 0;
}
