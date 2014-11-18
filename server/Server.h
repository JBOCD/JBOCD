#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#ifndef SERVER_H
#define SERVER_H

// MAX open file = 1024
// stdin, stdout, stderr --> 0-2
// Master file desp --> 3-9
// Maximum thread in process
// MAX_CLIENT + MAX_FORK + "0-2" + "3-9" == MAX open file
// for maximum connection, MAX_CLIENT == MAX_FORK
// at current stage, not implement fork yet.
#define MAX_CLIENT 1014
#define MAX_FORK 0
struct client_info{
	struct sockaddr_in conn;
	pthread_t thread;
	int connfd;
	int sockLen;
};
class Server{
	private:
		Config* conf;
		short port;
		static void* client_thread(void*);
	public:
		Server(Config*);
};

#include "Server.cpp"

#endif