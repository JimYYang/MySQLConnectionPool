#include "pch.h"
#include "CommonConnectionPool.h"
#include "public.h"
#include <iostream>
#include <string>

using namespace std;

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	// 对于其他类型的静态对象（如全局静态对象或类静态成员），它们的初始化不保证线程安全
	static ConnectionPool pool; // 对静态对象的初始化  编译器自动lock和unlock
	return &pool; // 静态对象编译时定义 第一次运行时初始化
}

// 从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}
	while (!feof(pf))
	{
		char line[1024] = { 0 };
		// 要么读满 要么读到换行符
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == string::npos)
		{
			continue;
		}

		// password=123456\n
		int endIdx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endIdx - idx - 1);
		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = stoi(value);
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = stoi(value);
		}
		else if (key == "maxSize")
		{
			_maxSize = stoi(value);
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = stoi(value);
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = stoi(value);
		}
	}
	return true;
}

// 连接池的构造
ConnectionPool::ConnectionPool()
{
	// 加载配置项
	if (!loadConfigFile())
	{
		return;
	}

	// 创建初始数量的连接
	// 这是启动连接池 还没有线程 所以不用考虑线程安全
	// 当启动完毕 开始运作 不同的线程才会对连接池队列进行操作
	for (int i = 0; i < _initSize; i++)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime(); // 刷新一下开始空闲的起始时间
		_connectionQue.push(p);
		_connectionCnt++;
	}

	// 启动一个新的线程 作为连接的生产者
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	// 设置为守护线程 主线程结束 守护线程自动结束
	produce.detach();

	// 启动一个新的定时线程 扫描超过maxIdleTime的空闲连接 进行多余空闲连接的回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock); // 队列不空 此处生产线程进入等待状态
		}

		// 连接数量没有到达上限 继续创建新连接
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime(); // 刷新一下开始空闲的起始时间
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// 通知消费者线程 可用消费连接了
		cv.notify_all();
	}
}


// 扫描超过maxIdleTime的空闲连接 进行多余空闲连接的回收
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// 通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		// 扫描整个队列 释放多余的连接
		// 释放连接涉及到删除 需要保证线程安全
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= _maxIdleTime * 1000)
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p; // 调用Connection析构函数释放连接
			}
			else
			{
				break; // 对头都没有超过 那都不会超过
			}
		}
	}
}

// 给外部提供接口 从连接池获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		// 这里可能被唤醒了 但是还没取到连接 被别的线程抢走了
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("获取空闲连接超时了...获取连接失败");
				return nullptr;
			}
		}
	}

	/*
	* shared_ptr 智能指针析构时 会把connection资源直接delete掉
	* 相当于调用connection的析构函数 connection就被close掉
	* 需要自定义shared_ptr的释放资源方式 把connection直接归还到队列中
	*/

	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection *pconn) {
			// 这里是在服务器应用线程中调用的 因此一定要考虑队列的线程安全
			unique_lock<mutex> lock(_queueMutex); // 出作用域析构 释放锁
			pconn->refreshAliveTime(); // 刷新一下开始空闲的起始时间
			_connectionQue.push(pconn);
		});
	_connectionQue.pop();
	//if (_connectionQue.empty())
	//{
	//	cv.notify_all(); // 谁消费了队列的最后一个连接 谁负责通知生产者生产连接
	//}
	cv.notify_all(); // 通知其他人消费 通知生产者检查一下 如果队列空了 则生产新连接
	return sp;
}
