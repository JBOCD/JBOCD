#include <cstdio>

#include "Config.h"
#include "MemManager.h"
#include "FileManager.h"
#include "MySQL.h"
#include "Network.h"
#include "Thread.h"
#include "WebSocket.h"
#include "Server.h"

using namespace std;

int main() {
	Config* config=new Config();

	// connect to DB
	MySQL* db = new MySQL(config);

	// init Memory Manager
	MemManager::init(config);

	// init File Manager
	FileManager::init(config);

	// init WebSocket
	WebSocket::init(config);
	
	// start server
	new Server(config);

	delete db;
}

