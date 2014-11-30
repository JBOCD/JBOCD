GoogleDrive::GoogleDrive(char* accessToken, int id){
	int i=0;
	for(;*(accessToken+i);i++);
	this->accessToken = malloc(i);
	memcpy(this->accessToken, accessToken, i);
	tmpStr = (char*) malloc(4096); // 4KB block
	this->id = id;
}
int GoogleDrive::get(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s %s %s %s",
		"/var/JBOCD/module/googledrive/get.py",
		accessToken,
		remotefilePath,
		localfilePath
	);
	return system(tmpStr);
}
int GoogleDrive::put(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s %s %s %s",
		"/var/JBOCD/module/googledrive/put.py",
		accessToken,
		localfilePath,
		remotefilePath
	);
	return system(tmpStr);
}
int GoogleDrive::ls(char* localfilePath){
	sprintf(tmpStr, "%s %s %s",
		"/var/JBOCD/module/googledrive/list.py",
		accessToken,
		localfilePath
	);
	return system(tmpStr);
}
int GoogleDrive::del(char* remotefilePath){
	sprintf(tmpStr, "%s %s %s",
		"/var/JBOCD/module/googledrive/list.py",
		accessToken,
		remotefilePath
	);
	return system(tmpStr);
}
bool GoogleDrive::isID(int id){
	return this->id == id;
}

GoogleDrive::~GoogleDrive(){
	if(tmpStr){
		free(tmpStr);
		tmpStr = 0;
	}
}