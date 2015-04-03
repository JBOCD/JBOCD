#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "CDDriver.h"

extern "C" CDDriver* createObject(const char*, unsigned int);
class GoogleDrive: public CDDriver{
	protected:
		static const char* lang;
		static const char* getCMD;
		static const char* putCMD;
		static const char* listCMD;
		static const char* delCMD;
		char* tmpStr;
		char* accessToken;
		unsigned int id;
	public:
		~GoogleDrive();
		GoogleDrive(const char* accessToken, unsigned int id);
		int get(char* remotefilePath, char* localfilePath);
		int put(char* remotefilePath, char* localfilePath);
		int ls (char* remotefilePath, char* localfilePath);
		int del(char* remotefilePath);
		bool isID(unsigned int id);
		unsigned int getID();
};
