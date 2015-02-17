void MemManager::init(Config* conf){
	free_list.next = NULL;
	mutex = PTHREAD_MUTEX_INITIALIZER;
	maxAllocate = (unsigned int)json_object_get_int(conf->get("memory.maxAllocate"));
}

void* MemManager::allocate(unsigned int size){
	MemManager::allocate(unsigned int size, false);
}
void* MemManager::allocate(unsigned int size, bool isExactSize){
	struct mem_header* result, tmpPre = &free_list, tmpCur;
	pthread_mutex_lock(&mutex);
	tmpCur = free_list.next
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
		// not yet implement limitation
		if(sizeof(struct mem_header) + size > maxAllocate - allocateSize){
			MemManager::flush(sizeof(struct mem_header) + size - allocateSize);
		}
		if(result = malloc(sizeof(struct mem_header) + size)){
			allocateSize+= size;
			result->size = size;
		}
	}
	return result ? (void*)(result + 1) : NULL;
}

void MemManager::free(void* mem){
	struct mem_header* free_mem = ((struct mem_header*) mem)-1;
	struct mem_header* tmpPre = &free_list, tmpCur;
	pthread_mutex_lock(&mutex);
	tmpCur = free_list.next;
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
	}
	if(free_mem){
		tmpPre->next = free_mem;
	}
	pthread_mutex_unlock(&mutex);
}
void MemManager::flush(unsigned int targetSize){
	struct mem_header* tmpCur;
	pthread_mutex_lock(&mutex);
	tmpCur = free_list.next;
	allocateSize
	while(tmpCur && targetSize > 0){
		if(tmpCur->other){
			free_list.next = tmpCur->other;
			tmpCur->other->next = tmpCur->next;
		}else{
			free_list.next = tmpCur->next;
		}
		targetSize -= tmpCur->size;
		free(tmpCur);
		tmpCur = free_list.next;
	}
	pthread_mutex_unlock(&mutex);
}