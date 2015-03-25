void Thread::init(){
	maxThread = json_object_get_int(Config::get("server.maxThreadPerConnection"));
	maxThread = maxThread < 1 ? 1 : maxThread;
	pthread_create(&create_thread_tid, NULL, &Thread::createThreadFromQueue, NULL); // thread end only when process end
}

void* Thread::createThreadFromQueue(void* arg){
	// implement some priority algorithm for priority queue
	struct thread_info* tmp;
	do{
		pthread_mutex_lock(&new_thread_mutex);
		if(!createRoot) continue;
		pthread_mutex_lock(&counter_mutex);
		if(maxThread <= curThread){
			pthread_mutex_unlock(&counter_mutex);
			Thread::clearThread();
			continue;
		}
		curThread++;
		pthread_mutex_unlock(&counter_mutex);
		pthread_mutex_lock(&new_thread_queue_mutex);
		createRoot = (tmp = createRoot)->next;
		pthread_mutex_unlock(&new_thread_queue_mutex);
		pthread_mutex_unlock(&new_thread_mutex);
		pthread_create(&(tmp->tid), NULL, &Thread::newThreadInit, (void*) tmp);
	}while(1);
}

void Thread::create(void* (*callback)(void*), void* arg){
	struct thread_info* thread = (struct thread_info*) MemManager::allocate(sizeof(struct thread_info));
	thread->cb = callback;
	thread->arg = arg;
	thread->next = NULL;
	pthread_mutex_lock(&new_thread_queue_mutex);
	if(createRoot){
		createRootLast = (createRootLast->next = thread);
	}else{
		createRootLast = (createRoot = thread);
	}
	pthread_mutex_unlock(&new_thread_queue_mutex);
	pthread_mutex_unlock(&new_thread_mutex);
}

void* Thread::newThreadInit(void* info){
	struct thread_info* thread = (struct thread_info*) info;
	pthread_t t = thread->tid;
	(*thread->cb)(thread->arg);
	MemManager::free(info);
	Thread::addDelQueue(t);
}

void Thread::addDelQueue(pthread_t t){
	struct thread_queue* val = (struct thread_queue*) MemManager::allocate(sizeof(struct thread_queue));
	printf("[%05d] Added Ended Thread to queue.\n", getpid());
	val->t = t;
	pthread_mutex_lock(&del_queue_mutex);
	val->next = delRoot;
	delRoot = val;
	pthread_mutex_unlock(&del_queue_mutex);
}
void Thread::clearThread(){
	struct thread_queue* tmp;
	pthread_mutex_lock(&del_queue_mutex);
	pthread_mutex_lock(&counter_mutex);
	while(delRoot){
		tmp = delRoot;
		printf("[%05d] Clear Ended thread (%d).\n", (int)getpid(), (int) tmp->t);
		pthread_join(tmp->t, NULL);
		curThread--;
		delRoot = delRoot->next;
		MemManager::free(tmp);
		pthread_mutex_unlock(&new_thread_mutex);
	}
	pthread_mutex_unlock(&counter_mutex);
	pthread_mutex_unlock(&del_queue_mutex);
}
