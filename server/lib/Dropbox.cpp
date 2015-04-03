#include "Dropbox.h"
const char* Dropbox::lang   ="python";
const char* Dropbox::getCMD ="/var/JBOCD/module/dropbox/get.py";
const char* Dropbox::putCMD ="/var/JBOCD/module/dropbox/put.py";
const char* Dropbox::listCMD="/var/JBOCD/module/dropbox/list.py";
const char* Dropbox::delCMD ="/var/JBOCD/module/dropbox/delete.py";

Dropbox::Dropbox(const char* accessToken, unsigned int id){
	int len = strlen(accessToken);
	this->accessToken = (char*) malloc(len+1);
	strcpy(this->accessToken, accessToken);
	tmpStr = (char*) malloc(1024+len);
	this->id = id;
}
int Dropbox::get(char* remotefilePath, char* localfilePath){
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
int Dropbox::put(char* remotefilePath, char* localfilePath){
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
int Dropbox::ls(char* remotefilePath, char* localfilePath){
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
int Dropbox::del(char* remotefilePath){
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
