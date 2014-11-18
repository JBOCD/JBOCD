Config::Config(){
	FILE *fPtr=fopen("config.json", "r");
	char *buffer;
	unsigned long fsize;

	nul = json_tokener_parse("null");

	if(fPtr==NULL){
		fprintf(stderr,"/etc/JBOCD/config.json is not found.\n");
		exit(1);
	}else{
		fseek(fPtr, 0, SEEK_END);
		fsize=ftell(fPtr)+1;
		fseek(fPtr, 0, SEEK_SET);

		buffer=(char*)malloc(fsize);
		if(buffer == NULL){
			fprintf(stderr,"Cannot allocate memory.\n");
			exit(1);
		}else{
			fread(buffer, sizeof(char), fsize, fPtr);
			root = json_tokener_parse(buffer);

			free(buffer);
		}
		fclose(fPtr);
	}
}
json_object *Config::getFrom(const char *path, json_object *curRoot){
	json_object *tmp;
	char * originalPath = path;
	unsigned char i=0, flag=0;
	do{
		if(flag)path++ && i-- && (flag=0);
		if(*(path+i) == 0 || *(path+i) == '.' || (i && *(path+i)=='[')){
			if(i>CONFIG_MAX_KEY){
				fprintf(stderr, "Config Path Error: key should not exceed 20 character. %s\n", originalPath);
				exit(1);
			}
			if(i==0){
				fprintf(stderr, "Config Path Error: key should not be empty. %s\n", originalPath);
				exit(1);
			}
			memcpy(tmpStr, path, i);
			tmpStr[i]=0;
			json_object_object_get_ex(curRoot, tmpStr, &tmp);
			curRoot=tmp;
			path += i;
			i=0;
			flag=1;
		}
		if(*(path+i)=='['){
			if(json_object_get_type(curRoot) == json_type_array){
				unsigned char j=0;
				int index;
				path+=i+1;
				while(*(path+j)!=']'){
					if(*(path+j)<'0' || *(path+j)>'9'){
						fprintf(stderr, "Config Path Error: array index is not an integer. %s\n", originalPath);
						exit(1);
					}
					j++;
				}
				if(j>10){
					fprintf(stderr, "Config Path Error: array index too long. %s\n", originalPath);
					exit(1);
				}
				if(j==0){
					fprintf(stderr, "Config Path Error: array index should not be empty. %s\n", originalPath);
					exit(1);
				}
				memcpy(tmpStr, path, j);
				tmpStr[j]=0;
				index=atoi(tmpStr);
				curRoot=json_object_array_get_idx(curRoot, index);
				path+=*(path+j+1)=='['?j:j+1;
				flag=1;
			}else{
				curRoot=nul;
			}
		}
	}while(*(path+i++) && json_object_get_type(curRoot));
	return curRoot;
}

json_object *Config::get(const char * path){
	return getFrom(path, root);
}
