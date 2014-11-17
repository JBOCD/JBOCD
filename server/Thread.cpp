
void Thread::addDelQueue(struct thread_queue* val, pthread_t t, void* res){
	val->t = t;
	val->res = res;
	val->next = root;
	pthread_mutex_lock(&mutex);
	root = val;
	pthread_mutex_unlock(&mutex);
}
void Thread::clearThread(){
	struct thread_queue* tmp;
	pthread_mutex_lock(&mutex);
	while(root){
		tmp = root;
		pthread_join(tmp->t);
		root = root->next;
		free(tmp->res);
		free(tmp);
	}
	pthread_mutex_unlock(&mutex);
}