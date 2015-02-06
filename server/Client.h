#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>

#include "lib/GoogleDrive.h"
#include "lib/Dropbox.h"


#ifndef SERVER_H
#define SERVER_H

// MAX open file = 1024
// stdin, stdout, stderr --> 0-2
// Master file desp --> 3-9
// Maximum thread in process
// MAX_CLIENT + MAX_FORK + "0-2" + "3-9" == MAX open file
// for maximum connection, MAX_CLIENT == MAX_FORK
// at current stage, not implement fork yet.
struct client_info{
	struct sockaddr_in conn;
	pthread_t thread;
	int connfd;
	int sockLen;
};
class Client{
	private:
		Config* conf;
		struct client_info* conn_conf;

// Cloud Drive API
		CDDriver ** cloudDrives; // new
		CDDriver ** dropboxList; // deprecated
		CDDriver ** googleDriveList; // deprecated

// WebSocket
		unsigned char maskKey[4];
		unsigned char* buffer;
		long long recvLen; // receive length == A WebSocket packet payload length
		int readLen; // read length == how many bytes of the WebSocket packet read into buffer

// function
		void load_CDDriver();
		void doHandshake();
	public:
		Client(Config*, struct client_info*);
		static void _client_close(int signum);
};

#include "Server.cpp"

#endif
