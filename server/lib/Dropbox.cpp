#include "Dropbox.h"
Dropbox::Dropbox(const char* accessToken, unsigned int id){
	int len = strlen(accessToken);
	this->accessToken = (char*) malloc(len+1);
	strcpy(this->accessToken, accessToken);
	tmpStr = (char*) malloc(1024+len);
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
		"python /var/JBOCD/module/dropbox/delete.py",
		accessToken,
		remotefilePath
	);
	return system(tmpStr);
}
bool Dropbox::isID(unsigned int id){
	return this->id == id;
}
unsigned int Dropbox::getID(){
	return this->id;
}

Dropbox::~Dropbox(){
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
	return new Dropbox(accessToken, id);
}
