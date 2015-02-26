#ifndef DROPBOX_H
#define DROPBOX_H

#include "CDDriver.h"

extern "C" CDDriver* createObject(const char*, unsigned int);
class Dropbox: public CDDriver{
	protected:
		char* tmpStr;
		char* accessToken;
		unsigned int id;
	public:
		~Dropbox();
		Dropbox(const char* accessToken, unsigned int id);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* remotefilePath, char* localfilePath);
		int del(char* remotefilePath);
		bool isID(unsigned int id);
		unsigned int getID();
};


#include "Dropbox.cpp"

#endif