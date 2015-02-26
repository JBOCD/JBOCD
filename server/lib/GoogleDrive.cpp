GoogleDrive::GoogleDrive(const char* accessToken, unsigned int id){
	int len = strlen(accessToken);
	this->accessToken = (char*) malloc(len+1);
	strcpy(this->accessToken, accessToken);
	tmpStr = (char*) malloc(1024+len);
	this->id = id;
}
int GoogleDrive::get(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s '%s' '%s' '%s'",
		"python /var/JBOCD/module/googledrive/get.py",
		accessToken,
		remotefilePath,
		localfilePath
	);
	return system(tmpStr);
}
int GoogleDrive::put(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s '%s' '%s' '%s'",
		"python /var/JBOCD/module/googledrive/put.py",
		accessToken,
		localfilePath,
		remotefilePath
	);
	return system(tmpStr);
}
int GoogleDrive::ls(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s '%s' '%s' '%s'",
		"python /var/JBOCD/module/googledrive/list.py",
		accessToken,
		localfilePath,
		remotefilePath
	);
	return system(tmpStr);
}
int GoogleDrive::del(char* remotefilePath){
	sprintf(tmpStr, "%s '%s' '%s'",
		"python /var/JBOCD/module/googledrive/delete.py",
		accessToken,
		remotefilePath
	);
	return system(tmpStr);
}
bool GoogleDrive::isID(unsigned int id){
	return this->id == id;
}
unsigned int GoogleDrive::getID(){
	return this->id;
}

GoogleDrive::~GoogleDrive(){
	if(tmpStr){
		free(tmpStr);
		tmpStr = 0;
	}
	if(accessToken){
		free(accessToken);
		accessToken = 0;
	}
}

CDDriver* createObject(const char* accessToken, unsigned int id){
	return new GoogleDrive(accessToken, id);
}