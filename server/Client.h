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

class Client{
	private:

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

		struct client_thread_director{
			Client* object;
			void (Client::*fptr)(void*);
		};
		// 0x02
		struct client_clouddrive_root{
			unsigned char operationID;
			unsigned short numOfCloudDrives;
			CDDriver ** root;
		};

		// 0x03
		struct client_clouddrive{
			unsigned int cdid;
			unsigned long long size;
			char* dir;
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
			unsigned int ldid;
			struct client_list* root;
		};

		// 0x20
		struct client_make_file{
			unsigned char operationID;
			unsigned int ldid;
			unsigned long long fileid;
			char* name;
		};

		// 0x21
		struct client_save_file{
			Client* object;
			void (Client::*fptr)(void*);
			unsigned char operationID;
			unsigned int ldid;
			unsigned int cdid;
			unsigned long long fileid;
			unsigned int seqnum;
			char* remoteName;
			unsigned int* tmpFile;
			unsigned int chunkSize;
			unsigned char status;
		};

		// 0x22
		struct client_read_file_info{
			unsigned char operationID;
			unsigned int num_of_chunk;
			unsigned long long size;
			unsigned int ldid;
			unsigned long long fileID;
		};
		// 0x23
		struct client_read_file{
			Client* object;
			void (Client::*fptr)(void*);
			unsigned char operationID;
			unsigned int ldid;
			unsigned int cdid;
			unsigned long long fileid;
			unsigned int seqnum;
			char* chunkName;
			unsigned int* tmpFile;
			unsigned int status;
			unsigned int chunkSize;
		};

		//0x28
		struct client_del_chunk{
			Client* object;
			void (Client::*fptr)(void*);
			CDDriver* cd;
			char*  dir;
			char* chunkName;
//			unsigned char status;
		};
		// 0x28, 0x29
		struct client_del_file{
			unsigned char operationID;
			unsigned int ldid;
			unsigned long long parentid;
			unsigned long long fileid;
			char* name;
		};

		pthread_t responseThread_tid;

// Client Info
		unsigned int account_id;
// Cloud Drive Info
		struct client_clouddrive_root* cd_root;

// Logical Drive Info
		struct client_logical_drive_root* ld_root;

// Client End
		pthread_mutex_t client_end_mutex;

// CloudDriver handler list
		struct clouddriver_handler_list* cd_handler;

// WebSocket
		int err;
		unsigned char maskKey[4];
		unsigned char* inBuffer;
		unsigned char* outBuffer;
		int readLen; // read length == how many bytes of the WebSocket packet read into buffer

// reponse thread
		struct client_response* res_root;
		struct client_response* res_last;
		pthread_mutex_t res_mutex;
		pthread_mutex_t res_queue_mutex;

// prepareStatement List
		sql::PreparedStatement* get_cloud_drive_list;
		sql::PreparedStatement* get_number_of_library;
		sql::PreparedStatement* get_list;
		sql::PreparedStatement* get_next_fileid;
		sql::PreparedStatement* create_file;
		sql::PreparedStatement* check_chunk_size;
		sql::PreparedStatement* check_clouddrive_size;
		sql::PreparedStatement* check_logicaldrive_size;
		sql::PreparedStatement* update_chunk_info;
		sql::PreparedStatement* update_clouddrive_alloc_size;
		sql::PreparedStatement* get_file_chunk;
		sql::PreparedStatement* get_child;
		sql::PreparedStatement* get_file;
		sql::PreparedStatement* del_file;
		sql::PreparedStatement* get_all_chunk;
		sql::PreparedStatement* remove_chunk;

		sql::ResultSet *res;

// const
		// file chunk upload / update
		static const char DELETE;
		static const char UPDATE;
		static const char INSERT;
		static const char NO_CHANGE;
		static const char CHUNK_SIZE_EXCEED_CD_LIMIT;
		static const char CHUNK_SIZE_EXCEED_LD_LIMIT;
		static const char CHUNK_SIZE_ZERO_EXCEPTION;
		static const char CD_NOT_IN_LD;
		static const char RETRY_LIMIT_EXCEED;
		static const char PERMISSION_DENY;

// config
		unsigned char maxPutTry;
		unsigned char maxGetTry;
		unsigned char maxDelTry;

// function
		void loadCloudDrive();
		void loadLogicalDrive();
		bool checkLogicalDrive(unsigned int ldid);

		void prepareStatement();
		void doHandshake();

		void commandInterpreter();

		void addResponseQueue(unsigned char command, void* info);
		void responseThread(void* arg);

		void readLogin();
		void readGetService();
		void readGetDrive();
		void readList();
		void readCreateFile();
		void readSaveFile();
		void readGetFile();
		void readDelFile();
		void readDelChunk();

		void processSaveFile(void *arg);
		void processGetFileChunk(void *arg);
		void processDelChunk(void *arg);

		void sendLogin(unsigned char command, void* a);
		void sendGetCloudDrive();
		void sendGetLogicalDrive();
		void sendList(void* a);
		void sendCreateFile(void* a);
		void sendSaveFile(void* a);
		void sendGetFileInfo(void* a);
		void sendGetFileChunk(void* a);
		void sendDelFile(void* a);
		void sendDelChunk(void* a);
	public:
		Client();
		~Client();
		static void _client_close(int signum);
		static void* _thread_redirector(void* arg);
};

const char Client::DELETE = 3;
const char Client::UPDATE = 2;
const char Client::INSERT = 1;
const char Client::NO_CHANGE = 0;
const char Client::CHUNK_SIZE_EXCEED_CD_LIMIT = -1;
const char Client::CHUNK_SIZE_EXCEED_LD_LIMIT = -2;
const char Client::CHUNK_SIZE_ZERO_EXCEPTION = -3;
const char Client::CD_NOT_IN_LD = -4;
const char Client::RETRY_LIMIT_EXCEED = -5;
const char Client::PERMISSION_DENY = 0x80; // MIN value

#include "Client.cpp"

#endif
