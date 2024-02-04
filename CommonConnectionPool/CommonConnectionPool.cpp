#include "pch.h"
#include "CommonConnectionPool.h"
#include "public.h"
#include <iostream>
#include <string>

using namespace std;

// �̰߳�ȫ���������������ӿ�
ConnectionPool* ConnectionPool::getConnectionPool()
{
	// �����������͵ľ�̬������ȫ�־�̬������ྲ̬��Ա�������ǵĳ�ʼ������֤�̰߳�ȫ
	static ConnectionPool pool; // �Ծ�̬����ĳ�ʼ��  �������Զ�lock��unlock
	return &pool; // ��̬�������ʱ���� ��һ������ʱ��ʼ��
}

// �������ļ��м���������
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
		// Ҫô���� Ҫô�������з�
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

// ���ӳصĹ���
ConnectionPool::ConnectionPool()
{
	// ����������
	if (!loadConfigFile())
	{
		return;
	}

	// ������ʼ����������
	// �����������ӳ� ��û���߳� ���Բ��ÿ����̰߳�ȫ
	// ��������� ��ʼ���� ��ͬ���̲߳Ż�����ӳض��н��в���
	for (int i = 0; i < _initSize; i++)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime(); // ˢ��һ�¿�ʼ���е���ʼʱ��
		_connectionQue.push(p);
		_connectionCnt++;
	}

	// ����һ���µ��߳� ��Ϊ���ӵ�������
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	// ����Ϊ�ػ��߳� ���߳̽��� �ػ��߳��Զ�����
	produce.detach();

	// ����һ���µĶ�ʱ�߳� ɨ�賬��maxIdleTime�Ŀ������� ���ж���������ӵĻ���
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

// �����ڶ������߳��У�ר�Ÿ�������������
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock); // ���в��� �˴������߳̽���ȴ�״̬
		}

		// ��������û�е������� ��������������
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime(); // ˢ��һ�¿�ʼ���е���ʼʱ��
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// ֪ͨ�������߳� ��������������
		cv.notify_all();
	}
}


// ɨ�賬��maxIdleTime�Ŀ������� ���ж���������ӵĻ���
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// ͨ��sleepģ�ⶨʱЧ��
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		// ɨ���������� �ͷŶ��������
		// �ͷ������漰��ɾ�� ��Ҫ��֤�̰߳�ȫ
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= _maxIdleTime * 1000)
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p; // ����Connection���������ͷ�����
			}
			else
			{
				break; // ��ͷ��û�г��� �Ƕ����ᳬ��
			}
		}
	}
}

// ���ⲿ�ṩ�ӿ� �����ӳػ�ȡһ�����õĿ�������
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		// ������ܱ������� ���ǻ�ûȡ������ ������߳�������
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("��ȡ�������ӳ�ʱ��...��ȡ����ʧ��");
				return nullptr;
			}
		}
	}

	/*
	* shared_ptr ����ָ������ʱ ���connection��Դֱ��delete��
	* �൱�ڵ���connection���������� connection�ͱ�close��
	* ��Ҫ�Զ���shared_ptr���ͷ���Դ��ʽ ��connectionֱ�ӹ黹��������
	*/

	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection *pconn) {
			// �������ڷ�����Ӧ���߳��е��õ� ���һ��Ҫ���Ƕ��е��̰߳�ȫ
			unique_lock<mutex> lock(_queueMutex); // ������������ �ͷ���
			pconn->refreshAliveTime(); // ˢ��һ�¿�ʼ���е���ʼʱ��
			_connectionQue.push(pconn);
		});
	_connectionQue.pop();
	//if (_connectionQue.empty())
	//{
	//	cv.notify_all(); // ˭�����˶��е����һ������ ˭����֪ͨ��������������
	//}
	cv.notify_all(); // ֪ͨ���������� ֪ͨ�����߼��һ�� ������п��� ������������
	return sp;
}
