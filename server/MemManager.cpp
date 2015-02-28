void MemManager::init(){
	free_list->size = 0;
	free_list->next = NULL;
	maxAllocate = (unsigned int)json_object_get_int(Config::get("memory.maxAllocate"));
}

void* MemManager::allocate(unsigned int size){
	return MemManager::allocate(size, false);
}
void* MemManager::allocate(unsigned int size, bool isExactSize){
	struct mem_header* result = NULL, *tmpPre, *tmpCur;
	pthread_mutex_lock(&mutex);
	tmpCur = free_list->next;
	tmpPre = free_list;
	while(tmpCur){
		if(tmpCur->size == size || (tmpCur->size > size && !isExactSize)){
			result = tmpCur;
			if(tmpCur->other){
				tmpCur->other->next = tmpCur->next;
				tmpPre->next = tmpCur->other;
			}else{
				tmpPre->next = tmpCur->next;
			}
			break;
		}else if(tmpCur->size > size && isExactSize){
			break;
		}
		tmpPre = tmpCur;
		tmpCur = tmpCur->next;
	}
	pthread_mutex_unlock(&mutex);
	if(result == NULL){

		if(sizeof(struct mem_header) + size > maxAllocate - allocateSize){
			MemManager::flush(sizeof(struct mem_header) + size - allocateSize);
		}
		if(sizeof(struct mem_header) + size <=  maxAllocate - allocateSize){
			if(result = (struct mem_header*) malloc(sizeof(struct mem_header) + size)){
				allocateSize+= size;
				result->size = size;
				result->other = result->next = NULL;
			}
		}
	}
	return result ? (void*)(result + 1) : NULL;
}

void MemManager::free(void* mem){
	struct mem_header* free_mem = ((struct mem_header*) mem)-1;
	struct mem_header* tmpPre = free_list, *tmpCur = free_list->next;
	pthread_mutex_lock(&mutex);
	while(tmpCur){
		if(tmpCur->size > free_mem->size){
			free_mem->next = tmpCur;
			free_mem->other = NULL;
			tmpPre->next = free_mem;
			free_mem = NULL;
			break;
		}else if(tmpCur->size == free_mem->size){
			free_mem->next = tmpCur->next;
			free_mem->other = tmpCur;
			tmpPre->next = free_mem;
			free_mem = NULL;
			break;
		}
		tmpPre = tmpCur;
		tmpCur = tmpCur->next;
	}
	if(free_mem){
		tmpPre->next = free_mem;
	}
	pthread_mutex_unlock(&mutex);
}
void MemManager::flush(unsigned int targetSize){
	struct mem_header* tmpCur;
	pthread_mutex_lock(&mutex);
	tmpCur = free_list->next;
	
	while(tmpCur && targetSize > 0){
		if(tmpCur->other){
			free_list->next = tmpCur->other;
			tmpCur->other->next = tmpCur->next;
		}else{
			free_list->next = tmpCur->next;
		}
		allocateSize -= tmpCur->size;
		targetSize -= tmpCur->size;
		free(tmpCur);
		tmpCur = free_list->next;
	}
	pthread_mutex_unlock(&mutex);
}
