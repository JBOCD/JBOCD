#include <cstdio>

#include "Config.h"
#include "SecureSocket.h"
#include "MemManager.h"
#include "FileManager.h"
#include "Network.h"
#include "Thread.h"
#include "WebSocket.h"
#include "Server.h"

using namespace std;

int main() {
	Config::init();

	// SSL/TLS connection handler
	SecureSocket::init();

	// init Memory Manager
	MemManager::init();

	// init File Manager
	FileManager::init();

	// init WebSocket
	WebSocket::init();

	// start server
	new Server();
}

