#ifndef GOOGLEDRIVE_H
#define GOOGLEDRIVE_H

#include "CDDriver.h"

class GoogleDrive: public CDDriver{
	protected:
		char* tmpStr;
		char* accessToken;
	public:
		~GoogleDrive();
		GoogleDrive(char* accessToken);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* localfilePath);
		int del(char* remotefilePath);
};


#include "GoogleDrive.cpp"

#endif