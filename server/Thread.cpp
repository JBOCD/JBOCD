void Thread::addDelQueue(struct thread_queue* val, pthread_t t, void* res){
	val->t = t;
	val->res = res;
	pthread_mutex_lock(&mutex);
	val->next = root;
	root = val;
	pthread_mutex_unlock(&mutex);
	printf("Added Ended Connection to clear queue.\n")
}
void Thread::clearThread(){
	struct thread_queue* tmp;
	pthread_mutex_lock(&mutex);
	printf("Clearing ended connection ...\n");
	while(root){
		tmp = root;
		printf("Free memory of thread %d\n", (int) tmp->t);
		pthread_join(tmp->t, NULL);
		root = root->next;
		free(tmp->res);
		free(tmp);
	}
	printf("Clearing Completed.\n");
	pthread_mutex_unlock(&mutex);
}