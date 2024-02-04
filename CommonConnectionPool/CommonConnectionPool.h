#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include "Connection.h"

using namespace std;

/*
* 实现连接池功能模块
*/

class ConnectionPool
{
public:
	// 获取连接池对象实例
	static ConnectionPool* getConnectionPool();

	// 给外部提供接口 从连接池中获取一个可用的空闲连接
	// 不要直接给外部返回普通指针 否则还需要考虑回收问题
	// 直接返回智能指针 出作用域自动析构 重定义删除器 不要把连接释放掉
	shared_ptr<Connection> getConnection();
private:
	// 单例#1 构造函数私有化
	ConnectionPool(); 

	// 从配置文件中加载配置项
	bool loadConfigFile(); 

	// 运行在独立的线程中 专门负责生产新连接
	void produceConnectionTask();

	// 扫描超过maxIdleTime的空闲连接 进行多余空闲连接的回收
	void scannerConnectionTask();
	string _ip; // mysql的ip地址
	unsigned short _port; // mysql端口号 3306
	string _username; // mysql登录用户名
	string _password; // mysql登录密码
	string _dbname; // 连接数据库名称
	int _initSize; // 连接池初始连接量
	int _maxSize; // 连接池最大连接量
	int _maxIdleTime; // 连接池最大空闲时间
	int _connectionTimeout; // 连接池获取连接的超时时间

	// 由于多个线程在连接队列里取放连接，所以该操作必须是线程安全的
	queue<Connection*> _connectionQue; // 存储mysql连接的队列
	mutex _queueMutex; // 维护连接队列的线程安全互斥锁
	atomic_int _connectionCnt; // 记录多个线程连接创建的connection连接的总数量
	condition_variable cv; // 设置条件变量 用于连接生产线程和连接消费线程的通信
};
