#include <fcntl.h>
#include <unistd.h>
#include <json/json.h>
#include <pthread.h>

#ifndef FILE_MANAGEMER_H
#define FILE_MANAGEMER_H

#pragma pack(1)
struct file_store{
	struct file_store* next;
	int fileSize;
	int fd;
	unsigned int filename;
};
#pragma pop()

class FileManager{

	private:
		static int nameCount;
		static struct file_store* free_list;

		static unsigned int maxAllocate;
		static unsigned int curAllocate;

		static const char* dirpath;

		static pthread_mutex_t file_mutex;
		static pthread_mutex_t allocate_mutex;
		FileManager(){}
	public:
		static void init();
		static void newProcess();
		static void endProcess();
		static unsigned int* newTemp(unsigned int fileSize);
		static void deleteTemp(unsigned int* file_store);
		static void freeTemp();
		static int open(unsigned int* file, char mode);
		static void getTempPath(unsigned int* file, char* buf);
};

int FileManager::nameCount = 0;
struct file_store* FileManager::free_list = NULL;

unsigned int FileManager::maxAllocate = 0;
unsigned int FileManager::curAllocate = 0;

const char* FileManager::dirpath = NULL;

pthread_mutex_t FileManager::file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t FileManager::allocate_mutex = PTHREAD_MUTEX_INITIALIZER;

#include "FileManager.cpp"

#endif
