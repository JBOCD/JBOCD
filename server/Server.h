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
#include <fcntl.h>

#include "Client.h"

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
class Server{
	private:
		Config* conf;
		short port;
		pid_t child_pid;
		struct client_info* client_conf;
		static int conn_count;
		static int max_conn;
		static void* client_main();
	public:
		Server(Config*);
		static void _client_close(int signum);
		static void _server_close(int signum);
};

#include "Server.cpp"

#endif
