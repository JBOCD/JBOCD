#ifndef GOOGLEDRIVE_H
#define GOOGLEDRIVE_H

#include "CDDriver.h"

extern "C" CDDriver* createObject(const char*, int);
class GoogleDrive: public CDDriver{
	protected:
		char* tmpStr;
		char* accessToken;
		int id;
	public:
		~GoogleDrive();
		GoogleDrive(const char* accessToken, int id);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* remotefilePath, char* localfilePath);
		int del(char* remotefilePath);
		bool isID(int id);
};


#include "GoogleDrive.cpp"

#endif