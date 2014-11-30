#include <cstdio>

#include "Config.h"
#include "MySQL.h"
#include "Network.h"
#include "Thread.h"
#include "WebSocket.h"
#include "FileManager.h"
#include "Server.h"

using namespace std;

int main() {
	Config* config=new Config();
	char input[100];
	json_object *tmp;
/* config getter example
	while(1){
		scanf("%s",input);
		tmp = config->get(input);
		printf("%s, %d, %d, %d, %lf, %s\n", input, json_object_get_type(tmp), json_object_get_boolean(tmp), json_object_get_int(tmp), json_object_get_double(tmp), json_object_get_string(tmp));
	}
*/
	// connect to DB
	MySQL db = new MySQL(config);
	// init WebSocket
	WebSocket::init(config);
	// init FileManager
	FileManager::init(config);
	// start server
	new Server(config);
}

