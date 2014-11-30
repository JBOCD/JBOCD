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
		static sql::Driver* driver;
		static sql::Connection* con;
//		sql::Statement* stmt;
//		sql::Resultset* res;
	public:
		MySQL(Config*);
		static sql::Connection* getCon();
		~MySQL();
};
sql::Driver* MySQL::driver = 0;
sql::Connection* MySQL::con = 0;
#include "MySQL.cpp"

#endif