#include <json/json.h>
#include <pthread.h>

#ifndef FILE_MANAGEMER_H
#define FILE_MANAGEMER_H

#define CONFIG_MAX_KEY 20

struct file_store{
	int filename;
	struct file_store* next;
};

class FileManager{

	private:
		static int nameCount;
		static struct file_store* free_list;

		static const char* dirpath;

		static pthread_mutex_t mutex;
		FileManager(){}
	public:
		static void init(Config*);
		static int* newTemp();
		static void freeTemp(int* file_store);
		static void getTempPath(int*, char* buf);
};
struct file_store* FileManager::free_list = NULL;
pthread_mutex_t FileManager::mutex = PTHREAD_MUTEX_INITIALIZER;
int FileManager::nameCount = 0;
const char* FileManager::dirpath = 0;

#include "FileManager.cpp"

#endif