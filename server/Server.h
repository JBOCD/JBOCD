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

#include "MySQL.h"
#include "Client.h"

#ifndef SERVER_H
#define SERVER_H

class Server{
	private:
		short port;
		pid_t child_pid;
		struct client_info* client_conf;
		static int conn_count;
		static int max_conn;
	public:
		Server();
		static void _client_close(int signum);
		static void _server_close(int signum);
};

#include "Server.cpp"

#endif
