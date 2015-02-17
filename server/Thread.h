#include <pthread.h>

#ifndef THREAD_H
#define THREAD_H

struct thread_queue{
	struct thread_queue* next;
	pthread_t t;
};

struct thread_info{
	pthread_t tid;
	void* cb;
	void* arg;
};

class Thread{
	private:
		static struct thread_queue* root;
		static pthread_mutex_t del_queue_mutex;
		static pthread_mutex_t new_thread_mutex;
		static pthread_mutex_t counter_mutex;
		static int maxThread;
		static int curThread;

		static void addDelQueue(pthread_t t);
		static void newThreadInit(void* info);
		static void clearThread(); // only main thread can use this function
		Thread();
	public:
		static void init(Config* conf);
		static void create(void* callback, void* info);
};

struct thread_queue* Thread::root = NULL;
pthread_mutex_t Thread::del_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Thread::new_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Thread::counter_mutex = PTHREAD_MUTEX_INITIALIZER;

#include "Thread.cpp"

#endif