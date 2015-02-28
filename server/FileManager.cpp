void FileManager::init(){
	char* tmpStr = (char*) MemManager::allocate(512);
	nameCount = 0;

	dirpath = json_object_get_string(Config::get("file.temp.dirpath"));
	maxAllocate = json_object_get_int(Config::get("file.temp.maxAllocate"));

	sprintf(tmpStr, "rm -r %s*",dirpath);
	system(tmpStr); // -_-!!
	MemManager::free(tmpStr);
}

void FileManager::newProcess(){
	char* tmpStr = (char*) MemManager::allocate(512);
	curAllocate = 0;
	sprintf(tmpStr, "mkdir -p -m go=,u=rw %s%u",dirpath, (unsigned int)getpid());
	system(tmpStr); // -_-!!
	MemManager::free(tmpStr);
}
void FileManager::endProcess(){
	char* tmpStr = (char*) MemManager::allocate(512);
	sprintf(tmpStr, "rm -r %s%u",dirpath, (unsigned int)getpid());
	system(tmpStr); // -_-!!
	MemManager::free(tmpStr);
}


unsigned int* FileManager::newTemp(unsigned int fileSize){
	struct file_store* result = NULL;
	do{
		pthread_mutex_lock(&allocate_mutex);
		if(maxAllocate >= curAllocate + fileSize){
			curAllocate += fileSize;
			pthread_mutex_lock(&file_mutex);
			if(free_list){
				result = free_list;
				free_list = free_list->next;
			}else{
				result = (struct file_store*) MemManager::allocate(sizeof(struct file_store));
				result->filename = nameCount++;
			}
			pthread_mutex_unlock(&file_mutex);
		}
	}while(!result);
	pthread_mutex_unlock(&allocate_mutex);
	result->fileSize = fileSize;
	return (unsigned int*) (((void*)result)+sizeof(struct file_store*)-sizeof(unsigned int));
}
void FileManager::deleteTemp(unsigned int* file){
	struct file_store* tmp = (struct file_store*) (((void*)file)-(unsigned int)sizeof(struct file_store*)+(unsigned int)sizeof(unsigned int));
	pthread_mutex_lock(&file_mutex);
	tmp->next=free_list;
	free_list = tmp;
	curAllocate -= tmp->fileSize;
	pthread_mutex_unlock(&file_mutex);
	pthread_mutex_unlock(&allocate_mutex);
}

void FileManager::freeTemp(){
	struct file_store* tmp;
	pthread_mutex_lock(&file_mutex);
	while(free_list){
		tmp = free_list;
		free_list = free_list->next;
		MemManager::free(tmp);
	}
	pthread_mutex_unlock(&file_mutex);
}

int FileManager::open(unsigned int* file, char mode){
	struct file_store* tmp = (struct file_store*) (((void*)file)-(unsigned int)sizeof(struct file_store*)+(unsigned int)sizeof(unsigned int));
	char* tmpStr = (char*) MemManager::allocate(512);
	FileManager::getTempPath(file, tmpStr);
	tmp->fd = ::open(tmpStr, mode == 'r' ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC);
	MemManager::free(tmpStr);
	return tmp->fd;
}

void FileManager::getTempPath(unsigned int* file, char* buf){
	sprintf(buf,"%s%u/%u",dirpath,(unsigned int)getpid(),*file);
}