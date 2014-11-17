#include <pthread.h>

#ifndef THREAD_H
#define THREAD_H

typedef struct thread_queue{
	struct thread_queue* next;
	void* res;
	pthread_t t;
};

class Thread{
	private:
		static struct thread_queue* root = null;
		static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
		Thread();
	public:
		static void addDelQueue(struct thread_queue* val, pthread_t t, void* res);
		static void clearThread(); // only main thread can use this function
};

#include "Thread.cpp"

#endif