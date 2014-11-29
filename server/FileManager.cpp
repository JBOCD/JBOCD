void FileManager::init(Config* conf){
	dirpath = json_object_get_string(conf->get("file.temp.dirpath"));
	char* tmpStr = (char*) malloc(512);
	sprintf(tmpStr, "rm -r %s* ; mkdir -p %s",dirpath, dirpath);
	system(tmpStr); // -_-!!
	free(tmpStr);
}

int* FileManager::newTemp(){
	pthread_mutex_lock(&mutex);
	struct file_store* result;
	if(free_list){
		result = free_list;
		free_list = free_list->next;
	}else{
		result = (struct file_store*) malloc(sizeof(struct file_store));
		result->filename = nameCount++;
	}
	pthread_mutex_unlock(&mutex);
	return (int*) result;
}
void FileManager::freeTemp(int* file){
	struct file_store* tmp = (struct file_store*) file;
	pthread_mutex_lock(&mutex);
	tmp->next=free_list;
	free_list = tmp;
	pthread_mutex_unlock(&mutex);
}

void FileManager::getTempPath(int* file, char* buf){
	sprintf(buf,"%s%d",dirpath,*file);
}