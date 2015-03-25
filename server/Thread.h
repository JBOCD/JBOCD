#include <pthread.h>
#include <unistd.h>

#ifndef THREAD_H
#define THREAD_H

struct thread_queue{
	struct thread_queue* next;
	pthread_t t;
};

struct thread_info{
	struct thread_info* next;
	pthread_t tid;
	void* (*cb)(void*);
	void* arg;
};

class Thread{
	private:
		static struct thread_queue* delRoot;
		static struct thread_info* createRoot;
		static struct thread_info* createRootLast;
		static pthread_t create_thread_tid;
		static pthread_mutex_t del_queue_mutex;
		static pthread_mutex_t new_thread_mutex;
		static pthread_mutex_t new_thread_queue_mutex;
		static pthread_mutex_t counter_mutex;
		static int maxThread;
		static int curThread;

		static void addDelQueue(pthread_t t);
		static void* createThreadFromQueue(void* arg);
		static void* newThreadInit(void* info);
		static void clearThread(); // only main thread can use this function
		Thread();
	public:
		static void init();
		static void create(void* (*callback)(void*), void* info);
};

struct thread_queue* Thread::delRoot = NULL;
struct thread_info* Thread::createRoot = NULL;

struct thread_info* Thread::createRootLast = NULL;
pthread_t Thread::create_thread_tid = 0;

pthread_mutex_t Thread::del_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Thread::new_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Thread::new_thread_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Thread::counter_mutex = PTHREAD_MUTEX_INITIALIZER;

int Thread::maxThread = 0;
int Thread::curThread = 0;


#include "Thread.cpp"

#endif
