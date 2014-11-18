void Thread::init(){
	root = NULL;
	mutex = PTHREAD_MUTEX_INITIALIZER;
}

void Thread::addDelQueue(struct thread_queue* val, pthread_t t, void* res){
	val->t = t;
	val->res = res;
	pthread_mutex_lock(&Thread::mutex);
	val->next = Thread::root;
	Thread::root = val;
	pthread_mutex_unlock(&Thread::mutex);
}
void Thread::clearThread(){
	struct thread_queue* tmp;
	pthread_mutex_lock(&Thread::mutex);
	while(root){
		tmp = Thread::root;
		pthread_join(tmp->t, NULL);
		Thread::root = Thread::root->next;
		free(tmp->res);
		free(tmp);
	}
	pthread_mutex_unlock(&Thread::mutex);
}