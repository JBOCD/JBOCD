#include <cstdio>

#include "Config.h"
#include "MemManager.h"
#include "FileManager.h"
#include "Network.h"
#include "Thread.h"
#include "WebSocket.h"
#include "Server.h"

using namespace std;

int main() {
	puts("hi");

	Config::init();
	puts("hi");
	// init Memory Manager
	MemManager::init();
	puts("hi");

	// init File Manager
	FileManager::init();
	puts("hi");

	// init WebSocket
	WebSocket::init();
	puts("hi");

	// start server
	new Server();
}

