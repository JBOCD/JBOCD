#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "CDDriver.h"

extern "C" CDDriver* createObject(const char*, unsigned int);
class FTP: public CDDriver{
	protected:
		char* tmpStr;
		char* accessToken;
		unsigned int id;
	public:
		~FTP();
		FTP(const char* accessToken, unsigned int id);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* remotefilePath, char* localfilePath);
		int del(char* remotefilePath);
		bool isID(unsigned int id);
		unsigned int getID();
};
