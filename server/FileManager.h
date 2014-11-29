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
		int nameCount;
		struct file_store* free_list;

		char* dirpath;

		static pthread_mutex_t mutex;
		FileManager(){}
	public:
		static void init(Config*);
		static int* newTemp();
		static void freeTemp(int*);
};
struct file_store* FileManager::free_list = NULL;
pthread_mutex_t FileManager::mutex = PTHREAD_MUTEX_INITIALIZER;

#include "FileManager.cpp"

#endif