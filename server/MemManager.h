#include <stdlib.h>
#include <pthread.h>
#include <json/json.h>

#ifndef MEM_MANAGEMER_H
#define MEM_MANAGEMER_H

// 2 linked list structure
struct mem_header{
	int size;
	struct mem_header* next; // next size
	struct mem_header* other; // next same size memory
};

class MemManager{

	private:
		static struct mem_header free_list;
		static unsigned int maxAllocate;
		static unsigned int allocateSize;
		static pthread_mutex_t mutex;
		MemManager(){}
		static void flush(unsigned int targetSize);
	public:
		static void init();
		static void* allocate(unsigned int size, bool isExactSize);
		static void* allocate(unsigned int size);
		static void free(void* mem);
};

pthread_mutex_t MemManager::mutex = PTHREAD_MUTEX_INITIALIZER;

#include "MemManager.cpp"

#endif