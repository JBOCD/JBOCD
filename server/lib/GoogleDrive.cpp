#include "GoogleDrive.h"
const char* GoogleDrive::lang   ="python";
const char* GoogleDrive::getCMD ="/var/JBOCD/module/googledrive/get.py";
const char* GoogleDrive::putCMD ="/var/JBOCD/module/googledrive/put.py";
const char* GoogleDrive::listCMD="/var/JBOCD/module/googledrive/list.py";
const char* GoogleDrive::delCMD ="/var/JBOCD/module/googledrive/delete.py";

GoogleDrive::GoogleDrive(const char* accessToken, unsigned int id){
	int len = strlen(accessToken);
	this->accessToken = (char*) malloc(len+1);
	strcpy(this->accessToken, accessToken);
	tmpStr = (char*) malloc(1024+len);
	this->id = id;
}
int GoogleDrive::get(char* remotefilePath, char* localfilePath){
	pid_t pid;
	int status;
	if( (pid = fork()) ){
		waitpid(pid, &status, 0);
	}else{
		execlp(lang, lang, getCMD, accessToken, remotefilePath, localfilePath, NULL);
		printf("[%u] Fail to open script\n", getpid());
		exit(1);
	}
	return WEXITSTATUS(status);
}
int GoogleDrive::put(char* remotefilePath, char* localfilePath){
	pid_t pid;
	int status;
	if( (pid = fork()) ){
		waitpid(pid, &status, 0);
	}else{
		execlp(lang, lang, putCMD, accessToken, localfilePath, remotefilePath, NULL);
		printf("[%u] Fail to open script\n", getpid());
		exit(1);
	}
	return WEXITSTATUS(status);
}
int GoogleDrive::ls(char* remotefilePath, char* localfilePath){
	pid_t pid;
	int status;
	if( (pid = fork()) ){
		waitpid(pid, &status, 0);
	}else{
		execlp(lang, lang, listCMD, accessToken, localfilePath, remotefilePath, NULL);
		printf("[%u] Fail to open script\n", getpid());
		exit(1);
	}
	return WEXITSTATUS(status);
}
int GoogleDrive::del(char* remotefilePath){
	pid_t pid;
	int status;
	if( (pid = fork()) ){
		waitpid(pid, &status, 0);
	}else{
		execlp(lang, lang, delCMD, accessToken, remotefilePath, NULL);
		printf("[%u] Fail to open script\n", getpid());
		exit(1);
	}
	return WEXITSTATUS(status);
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
