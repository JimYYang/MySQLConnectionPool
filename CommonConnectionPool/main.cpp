#include "pch.h"
#include <iostream>
#include "Connection.h"
#include <iostream>

using namespace std;

int main()
{
	Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
		"zhang san", 20, "male");
	conn.connect("localhost", 3306, "root", "jsyanga@", "chat");
	conn.update(sql);
	return 0;
}
