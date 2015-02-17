void Thread::init(Config* conf){
	maxThread = json_object_get_int(conf->get("server.maxThreadPerConnection"));
	maxThread = maxThread < 1 ? 1 : maxThread;
}

void Thread::create(void* callback, void* arg){
	struct thread_info* thread;
	do{
		pthread_mutex_lock(&new_thread_mutex);
		pthread_mutex_lock(&counter_mutex);
		if(maxThread > curThread){
			curThread++;
			pthread_mutex_unlock(&counter_mutex);
			break;
		}
		pthread_mutex_unlock(&counter_mutex);
		Thread::clearThread();
	}while(1);
	thread = MemManager::allocate(sizeof(struct thread_info))
	thread->cb = callback;
	thread->arg = arg;
	pthread_create(&(thread->tid), NULL, &Thread::newThreadInit, (void*) thread));
}

void Thread::newThreadInit(void* info){
	struct thread_info* thread = (struct thread_info*) info;
	pthread_t t = thread;
	*thread->cb(thread->arg);
	MemManager::free(info);
	Thread::addDelQueue(tid);
}

void Thread::addDelQueue(pthread_t t){
	struct thread_queue* val = (struct thread_queue*) MemManager::allocate(sizeof(struct thread_queue));
	val->t = t;
	pthread_mutex_lock(&del_queue_mutex);
	val->next = root;
	root = val;
	pthread_mutex_unlock(&del_queue_mutex);
	printf("Added Ended Connection to clear queue.\n");
}
void Thread::clearThread(){
	struct thread_queue* tmp;
	pthread_mutex_lock(&del_queue_mutex);
	pthread_mutex_lock(&counter_mutex);
	printf("Clearing ended connection ...\n");
	while(root){
		tmp = root;
		printf("Free memory of thread %d\n", (int) tmp->t);
		pthread_join(tmp->t, NULL);
		curThread--;
		root = root->next;
		MemManager::free(tmp);
		pthread_mutex_unlock(&new_thread_mutex);
	}
	printf("Clearing Completed.\n");
	pthread_mutex_unlock(&counter_mutex);
	pthread_mutex_unlock(&del_queue_mutex);
}