#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/wait.h>
#include <pthread.h>
#include "CDDriver.h"

class Dropbox: public CDDriver{
	private:

		struct result_queue {
			unsigned int opID;
			unsigned int status;
			struct result_queue * next;
		};

		// c == client

		int c_stdin[2];
		int c_stdout[2];

		unsigned int opID;

		bool isRead;
		bool isReading;
		bool isClosed;

		pthread_mutex_t read_mutex;
		pthread_cond_t read_cond;

		pthread_mutex_t write_mutex;

		struct result_queue * root;
		struct result_queue * last;

		char *tmpStr;
	protected:
		int general(const char* command, const char* remoteFilePath, const char* localFilePath);
		unsigned int id;
	public:
		~Dropbox();
		Dropbox(const char* accessToken, unsigned int id);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int del(char* remotefilePath);
		bool isID(unsigned int id);
		unsigned int getID();
		static void* thread_reader(void* arg);
};
