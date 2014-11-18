#include <pthread.h>

#ifndef THREAD_H
#define THREAD_H

struct thread_queue{
	struct thread_queue* next;
	void* res;
	pthread_t t;
};

class Thread{
	private:
		static struct thread_queue* root;
		static pthread_mutex_t mutex;
		Thread();
	public:
		static void addDelQueue(struct thread_queue* val, pthread_t t, void* res);
		static void clearThread(); // only main thread can use this function
};

struct thread_queue* Thread::root = NULL;
pthread_mutex_t Thread::mutex = PTHREAD_MUTEX_INITIALIZER;

#include "Thread.cpp"

#endif