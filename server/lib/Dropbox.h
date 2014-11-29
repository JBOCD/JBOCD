#ifndef DROPBOX_H
#define DROPBOX_H

#include "CDDriver.h"

class Dropbox: public CDDriver{
	protected:
		char* tmpStr;
		char* accessToken;
	public:
		~Dropbox();
		Dropbox(char* accessToken);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* localfilePath);
		int del(char* remotefilePath);
};


#include "Dropbox.cpp"

#endif