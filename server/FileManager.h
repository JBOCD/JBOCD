#include <json/json.h>
#include <pthread.h>

#ifndef FILE_MANAGEMER_H
#define FILE_MANAGEMER_H

#define CONFIG_MAX_KEY 20

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
		static void init(Config*);
		static unsigned int* newTemp();
		static void deleteTemp(unsigned int* file_store);
		static void freeTemp();
		static int open(unsigned int* file, char mode);
		static void getTempPath(unsigned int* file, char* buf);
};

struct file_store* FileManager::free_list = NULL;
pthread_mutex_t FileManager::file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t FileManager::allocate_mutex = PTHREAD_MUTEX_INITIALIZER;

#include "FileManager.cpp"

#endif