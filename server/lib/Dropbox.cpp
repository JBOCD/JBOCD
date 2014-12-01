Dropbox::Dropbox(const char* accessToken, int id){
	int i=0;
	for(;*(accessToken+i);i++);
	this->accessToken = (char*) malloc(++i);
	memcpy(this->accessToken, accessToken, i);
	tmpStr = (char*) malloc(1024); // 512B block
	this->id = id;
}
int Dropbox::get(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s '%s' '%s' '%s'",
		"python /var/JBOCD/module/dropbox/get.py",
		accessToken,
		remotefilePath,
		localfilePath
	);
	return system(tmpStr);
}
int Dropbox::put(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s '%s' '%s' '%s'",
		"python /var/JBOCD/module/dropbox/put.py",
		accessToken,
		localfilePath,
		remotefilePath
	);
	return system(tmpStr);
}
int Dropbox::ls(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s '%s' '%s' '%s'",
		"python /var/JBOCD/module/dropbox/list.py",
		accessToken,
		localfilePath,
		remotefilePath
	);
	return system(tmpStr);
}
int Dropbox::del(char* remotefilePath){
	sprintf(tmpStr, "%s '%s' '%s'",
		"python /var/JBOCD/module/dropbox/list.py",
		accessToken,
		remotefilePath
	);
	return system(tmpStr);
}
bool Dropbox::isID(int id){
	return this->id == id;
}

Dropbox::~Dropbox(){
	if(tmpStr){
		free(tmpStr);
		tmpStr = 0;
	}
}