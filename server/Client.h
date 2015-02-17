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


#ifndef CLIENT_H
#define CLIENT_H

// MAX open file = 1024
// stdin, stdout, stderr --> 0-2
// Master file desp --> 3-9
// Maximum thread in process
// MAX_CLIENT + MAX_FORK + "0-2" + "3-9" == MAX open file
// for maximum connection, MAX_CLIENT == MAX_FORK
// at current stage, not implement fork yet.
struct client_info{
	struct sockaddr_in conn;
	int connfd;
	int sockLen;
};

struct response_thread{
	pthread_t tid;
	void *info;
};

struct client_response{
	unsigned char operationID;
	unsigned char command;
	void * info;
	struct client_response* next;
};

struct client_list{
	unsigned char operationID;
	int did;
	char* dirPath;
};
struct client_save_file{
	unsigned char operationID;
	unsigned int fid;
	unsigned int did;
	unsigned int sid;
	unsigned int seqnum;
	char* remoteName;
	unsigned int* tmpFile;
	int tmpFileID;
	unsigned int chunkSize;
};

class Client{
	private:
		Config* conf;
		struct client_info* conn_conf;
		pthread_t responseThread;

// Client Info
		int account_id=0;

// Cloud Drive API
		CDDriver ** cloudDrives; // new
		CDDriver ** dropboxList; // deprecated
		CDDriver ** googleDriveList; // deprecated

// WebSocket
		CDDriver ** tmpCDD;
		int err;
		bool isCont;
		bool isEnd;
		unsigned char maskKey[4];
		unsigned char* inBuffer;
		unsigned char* outBuffer;
		long long recvLen; // receive length == A WebSocket packet payload length
		int readLen; // read length == how many bytes of the WebSocket packet read into buffer

// reponse thread
		struct client_response* res_root;
		struct client_response* res_last;
		pthread_mutex_t res_mutex;
		pthread_mutex_t res_queue_mutex;


// function
		void load_CDDriver();
		void doHandshake();
		void commandInterpreter();
	public:
		Client(Config*, struct client_info*);
		static void _client_close(int signum);
};

#include "Client.cpp"

#endif
