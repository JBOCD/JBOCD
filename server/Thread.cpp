void Thread::init(){
	maxThread = json_object_get_int(Config::get("server.maxThreadPerConnection"));
	maxThread = maxThread < 1 ? 1 : maxThread;
	pthread_create(&create_thread_tid, NULL, &Thread::createThreadFromQueue, NULL); // thread end only when process end
	createRoot = (struct thread_info**)malloc(sizeof(struct thread_info*) * 6);
	createRootLast = createRoot+3;
	memset(createRoot, 0, sizeof(struct thread_info*) * 6);
}

void* Thread::createThreadFromQueue(void* arg){
	// implement some priority algorithm for priority queue
	static const unsigned char priorityFactor[3] = {5,4,2};
	static unsigned int priority[3] = {0,0,0};
	static unsigned char selected;
	struct thread_info* tmp;
	do{
		pthread_mutex_lock(&new_thread_mutex);
		while(!createRoot[0] && !createRoot[1] && !createRoot[2]){ // at least 1 thread
			pthread_cond_wait(&new_thread_cond, &new_thread_mutex);
		}
		pthread_mutex_unlock(&new_thread_mutex);

		if(maxThread <= curThread){
			Thread::clearThread();
		}
		curThread++;

		pthread_mutex_lock(&new_thread_queue_mutex);
		if(!createRoot[0] || (createRoot[1] || createRoot[2]) && priority[0]*priorityFactor[1] > priority[1]*priorityFactor[0]){
			if(!createRoot[1] || createRoot[2] && priority[1]*priorityFactor[2] > priority[2]*priorityFactor[1]){
				selected = 2;
			}else{
				selected = 1;
			}
		}else{
			selected = 0;
		}
		priority[selected] = priority[selected] + 1 % priorityFactor[selected];
		createRoot[selected] = (tmp = createRoot[selected])->next;
		pthread_mutex_unlock(&new_thread_queue_mutex);
		pthread_create(&(tmp->tid), NULL, &Thread::newThreadInit, (void*) tmp);
	}while(1);
}

void Thread::create(void* (*callback)(void*), void* arg, unsigned char priority){
	struct thread_info* thread = (struct thread_info*) MemManager::allocate(sizeof(struct thread_info));
	thread->cb = callback;
	thread->arg = arg;
	thread->next = NULL;
	pthread_mutex_lock(&new_thread_queue_mutex);
	if(priority >= 3){
		priority = 2;
	}
	if(createRoot[priority]){
		createRootLast[priority] = (createRootLast[priority]->next = thread);
	}else{
		createRootLast[priority] = (createRoot[priority] = thread);
	}
	pthread_mutex_lock(&new_thread_mutex);
	pthread_cond_signal(&new_thread_cond);
	pthread_mutex_unlock(&new_thread_mutex);

	pthread_mutex_unlock(&new_thread_queue_mutex);
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
	pthread_cond_signal(&del_queue_cond);
	pthread_mutex_unlock(&del_queue_mutex);
}
void Thread::clearThread(){ // call only in createThreadFromQueue, no need add counter mutex
	struct thread_queue* tmp;
	pthread_mutex_lock(&del_queue_mutex);
	if(!delRoot){
		pthread_cond_wait(&del_queue_cond, &del_queue_mutex);
	}
	while(delRoot){
		tmp = delRoot;
		printf("[%05d] Clear Ended thread (%u).\n", getpid(), (int) tmp->t);
		pthread_join(tmp->t, NULL);
		curThread--;
		delRoot = delRoot->next;
		MemManager::free(tmp);
	}
	pthread_mutex_unlock(&del_queue_mutex);
}
