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
* ʵ�����ӳع���ģ��
*/

class ConnectionPool
{
public:
	// ��ȡ���ӳض���ʵ��
	static ConnectionPool* getConnectionPool();

	// ���ⲿ�ṩ�ӿ� �����ӳ��л�ȡһ�����õĿ�������
	// ��Ҫֱ�Ӹ��ⲿ������ָͨ�� ������Ҫ���ǻ�������
	// ֱ�ӷ�������ָ�� ���������Զ����� �ض���ɾ���� ��Ҫ�������ͷŵ�
	shared_ptr<Connection> getConnection();
private:
	// ����#1 ���캯��˽�л�
	ConnectionPool(); 

	// �������ļ��м���������
	bool loadConfigFile(); 

	// �����ڶ������߳��� ר�Ÿ�������������
	void produceConnectionTask();

	// ɨ�賬��maxIdleTime�Ŀ������� ���ж���������ӵĻ���
	void scannerConnectionTask();
	string _ip; // mysql��ip��ַ
	unsigned short _port; // mysql�˿ں� 3306
	string _username; // mysql��¼�û���
	string _password; // mysql��¼����
	string _dbname; // �������ݿ�����
	int _initSize; // ���ӳس�ʼ������
	int _maxSize; // ���ӳ����������
	int _maxIdleTime; // ���ӳ�������ʱ��
	int _connectionTimeout; // ���ӳػ�ȡ���ӵĳ�ʱʱ��

	// ���ڶ���߳������Ӷ�����ȡ�����ӣ����Ըò����������̰߳�ȫ��
	queue<Connection*> _connectionQue; // �洢mysql���ӵĶ���
	mutex _queueMutex; // ά�����Ӷ��е��̰߳�ȫ������
	atomic_int _connectionCnt; // ��¼����߳����Ӵ�����connection���ӵ�������
	condition_variable cv; // ������������ �������������̺߳����������̵߳�ͨ��
};
