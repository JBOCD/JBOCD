#include <json/json.h>
#include <cstring>

#include <mysql_connection.h>
#include <mysql_driver.h>

//#include <cppconn/driver.h>
//#include <cppconn/exception.h>
//#include <cppconn/resultset.h>
//#include <cppconn/statement.h>

#ifndef MySQL_H
#define MySQL_H

class MySQL{
	private:
		sql::Driver* driver;
		sql::Connection* con;
//		sql::Statement* stmt;
//		sql::Resultset* res;
	public:
		MySQL(Config*);
		sql::Connection* getCon();
		~MySQL();
};

#include "MySQL.cpp"

#endif