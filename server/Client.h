#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#include <dlfcn.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/sqlstring.h>
#include <cppconn/prepared_statement.h> //http://forums.mysql.com/read.php?167,592107,592107

#include "lib/CDDriver.h"

#ifndef CLIENT_H
#define CLIENT_H

struct client_info{
	struct sockaddr_in conn;
	int connfd;
	int sockLen;
};

struct clouddriver_handler_list{
	void* handler;
	int lid;
	CDDriver* (*newCDDriver)(const char*, unsigned int);
};

struct client_response{
	unsigned char command;
	void * info;
	struct client_response* next;
};

// 0x02
struct client_clouddrive_root{
	unsigned char operationID;
	CDDriver ** root;
};

// 0x03
struct client_clouddrive{
	unsigned int cdid;
	unsigned long long size;
	struct client_clouddrive * next;
};
struct client_logical_drive{
	unsigned int ldid;
	unsigned int algoid;
	unsigned long long size;
	char* name;
	unsigned short numOfCloudDrives;
	struct client_logical_drive * next;
	struct client_clouddrive * root;
};
struct client_logical_drive_root{
	unsigned char operationID;
	unsigned short numOfLogicalDrive;
	struct client_logical_drive * root;
};

// 0x04
struct client_list{
	unsigned long long fileid;
	unsigned long long fileSize;
	char* name;
	struct client_list* next;
};
struct client_list_root{
	unsigned char operationID;
	unsigned short numberOfFile;
	struct client_list* root;
};

// 0x20
struct client_make_file{
	unsigned char operationID;
	unsigned long long fileid;
};

// 0x21
struct client_save_file{
	unsigned char operationID;
	unsigned int ldid;
	unsigned int cdid;
	unsigned long long fileid;
	unsigned int seqnum;
	char* remoteName;
	unsigned int* tmpFile;
	unsigned int chunkSize;
	unsigned char isInsertOK;
};

// 0x22
struct client_read_file_info{
	unsigned char operationID;
	unsigned int num_of_seq;
};
struct client_read_file{
	unsigned char operationID;
	unsigned int ldid;
	unsigned int cdid;
	unsigned long long fileid;
	unsigned int seqnum;
	unsigned char* chunkName;
	unsigned int chunkSize;
};

//0x28
struct client_del_file{
	unsigned char operationID;
	unsigned int ldid;
	unsigned long long fileid;
};

class Client{
	private:
		struct client_info* conn_conf;
		pthread_t responseThread;

// Client Info
		unsigned int account_id=0;

// Cloud Drive API
		unsigned short numOfCloudDrives=0;
		CDDriver ** cloudDrives; // new
//		CDDriver ** dropboxList; // deprecated
//		CDDriver ** googleDriveList; // deprecated

// CloudDriver handler list
		struct clouddriver_handler_list* cd_handler;
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

// prepareStatement List
		sql::PreparedStatement* check_user_logical_drive;
		sql::PreparedStatement* check_token;
		sql::PreparedStatement* get_cloud_drive_list;
		sql::PreparedStatement* get_number_of_library;
		sql::PreparedStatement* get_logical_drive;
		sql::PreparedStatement* get_cloud_drive_by_ldid;
		sql::PreparedStatement* get_list;
		sql::PreparedStatement* get_next_fileid;
		sql::PreparedStatement* create_file;
		sql::PreparedStatement* insert_chunk;
		sql::PreparedStatement* get_file_chunk;
// function
		void load_CDDriver();
		void doHandshake();
		void commandInterpreter();
	public:
		Client(struct client_info*);
		static void _client_close(int signum);
};

#include "Client.cpp"

#endif
