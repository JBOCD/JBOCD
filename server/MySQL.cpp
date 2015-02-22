void MySQL::init(){
	driver = sql::mysql::get_driver_instance();
	const char* host = json_object_get_string(Config::get("database.MySQL.host"));
	const char* port = json_object_get_string(Config::get("database.MySQL.port"));
	int strLen = 8+strlen(host)+strlen(port);
	char* conn = new char[strLen];
	snprintf(conn, strLen, "tcp://%s:%s", host, port);
	con = driver->connect(conn, json_object_get_string(Config::get("database.MySQL.user")), json_object_get_string(Config::get("database.MySQL.pass")));
	con->setSchema(json_object_get_string(Config::get("database.MySQL.dbname")));
	delete conn;
}
sql::Connection* MySQL::getCon(){
	return con;
}