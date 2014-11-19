MySQL::MySQL(Config* conf){
	driver = get_driver_instance();
	char* host = json_object_get_string(conf->get("database.MySQL.host"));
	char* port = json_object_get_string(conf->get("database.MySQL.port"));
	char* conn = new char[7+strlen(host)+strlen(port)+1];
	strncpy(strncpy(strncpy(strncpy(conn,"tcp://",6)+6,host,strlen(host))+strlen(host), ":",1)+1, port, strlen(port)); // tcp://host:port
	con = driver->connect(conn, json_object_get_string(conf->get("database.MySQL.user")), json_object_get_string(conf->get("database.MySQL.user")));
	con->setSchema(json_object_get_string(conf->get("database.MySQL.dbname")));
}
MySQL::~MySQL(){
	delete con;
}