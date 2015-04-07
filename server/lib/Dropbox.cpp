#include "Dropbox.h"

Dropbox::Dropbox(const char* accessToken, unsigned int id){
	pid_t pid;

	pthread_t tid;

	pipe(c_stdin);
	pipe(c_stdout);

	pthread_mutex_init(&read_mutex, NULL);
	pthread_cond_init(&read_cond, NULL);
	pthread_mutex_init(&write_mutex, NULL);

	if( ( pid = fork() ) > 0 ){
		close(c_stdin[0]);
		close(c_stdout[1]);

		opID = 0;
		isRead = false;
		isReading = false;
		isClosed = false;

		this->id = id;

		tmpStr = (char*) malloc(256);
		pthread_create(&tid, NULL, &Dropbox::thread_reader, this);

		root = NULL;
	}else if(pid == 0){
		close(c_stdin[1]);
		close(c_stdout[0]);

		dup2(c_stdin[0], STDIN_FILENO);
		close(c_stdin[0]);

		dup2(c_stdout[1], STDOUT_FILENO);
		close(c_stdout[1]);

		execlp("python", "python", "/var/JBOCD/module/dropbox/daemon.py", accessToken, NULL);
		exit(1);
	}else{
		printf("Can't fork a handler. Bye.\n");
		exit(1);
	}
}
int Dropbox::get(char* remoteFilePath, char* localFilePath){
	return general("get", remoteFilePath, localFilePath);
}
int Dropbox::put(char* remoteFilePath, char* localFilePath){
	return general("put", remoteFilePath, localFilePath);
}
int Dropbox::del(char* remoteFilePath){
	return general("delete", remoteFilePath, "");
}
int Dropbox::general(const char* command, const char* remoteFilePath, const char* localFilePath){
	unsigned int opID;
	int status;
	bool isDone = false;
	struct result_queue* pre_s;
	struct result_queue* s;
//	struct timeval now;
//	struct timespec timeout;

	pthread_mutex_lock(&write_mutex);
	opID = this->opID++;
	sprintf(
		tmpStr,
		"{\"opID\":%d, \"command\":\"%s\", \"remote\":\"%s\", \"local\":\"%s\"}\n",
		opID,
		command,
		remoteFilePath,
		localFilePath
	);
	write(c_stdin[1], tmpStr, strlen(tmpStr));
	pthread_mutex_unlock(&write_mutex);
	pthread_mutex_lock(&read_mutex);
	do{
		s = root;
		pre_s = NULL;
		while(s && !isDone){
			if(s->opID == opID){
				status = s->status;
				isDone = true;
				if(!pre_s){
					root = s->next;
				}else{
					pre_s->next = s->next;
				}
				free(s);
				break;
			}
//			printf("Not Match: s->opID = %u, s->status = %u, opID = %u\n", s->opID, s->status, opID);
			pre_s = s;
			s = s->next;
		}
//		gettimeofday(&now, NULL);
//		timeout.tv_sec = now.tv_sec+5;
//		timeout.tv_nsec = now.tv_usec*1000;
//		pthread_cond_timedwait(&read_cond, &read_mutex, &timeout);
		pthread_cond_wait(&read_cond, &read_mutex);
	}while(!isDone && !isClosed);
	pthread_mutex_unlock(&read_mutex);

	pthread_mutex_lock(&read_mutex);
	pthread_cond_broadcast(&read_cond);
	pthread_mutex_unlock(&read_mutex);

	return isClosed ? 500 : status;
}
bool Dropbox::isID(unsigned int id){
	return this->id == id;
}
unsigned int Dropbox::getID(){
	return this->id;
}

void* Dropbox::thread_reader(void* arg){
	Dropbox* that = (Dropbox*)arg;
	char a[] = {0,0};
	struct result_queue* r;
	while(1){
		r = (struct result_queue*) malloc(sizeof(struct result_queue));
		r->opID = r->status = 0;
		r->next = NULL;
		while(read(that->c_stdout[0], a, 1) && (a[0] < '0' || a[0] > '9')); // clear all non number char
		do{
			r->opID = r->opID * 10 + a[0] - '0';
		}while(read(that->c_stdout[0], a, 1) && a[0] >= '0' && a[0] <= '9');

		while(read(that->c_stdout[0], a, 1) && (a[0] < '0' || a[0] > '9')); // clear all non number char
		do{
			r->status = r->status * 10 + a[0] - '0';
		}while(!(that->isClosed = !read(that->c_stdout[0], a, 1)) && a[0] >= '0' && a[0] <= '9');

		pthread_mutex_lock(&that->read_mutex);
		if(that->root){
			that->last = (that->last->next = r);
		}else{
			that->last = (that->root = r);
		}
		pthread_cond_broadcast(&that->read_cond);
		pthread_mutex_unlock(&that->read_mutex);

	}
	return NULL;
}

Dropbox::~Dropbox(){
	if(tmpStr) free(tmpStr);
}

CDDriver* createObject(const char* accessToken, unsigned int id){
	return new Dropbox(accessToken, id);
}
