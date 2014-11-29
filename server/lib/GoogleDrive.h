#ifndef GOOGLEDRIVE_H
#define GOOGLEDRIVE_H

#include "CDDriver.h"

class GoogleDrive: public CDDriver{
	protected:
		char* tmpStr; 
	public:
		~GoogleDrive() : ~CDDriver();
		GoogleDrive(char* accessToken) : CDDriver(char* accessToken);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* localfilePath);
		int del(char* remotefilePath);
};


#include "GoogleDrive.cpp"

#endif