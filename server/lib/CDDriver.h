#ifndef CDDRIVER_H
#define CDDRIVER_H

class CDDriver{
	protected:
		char* accessToken;
	public:
		virtual ~CDDriver(){}
		CDDriver(char* accessToken);
		virtual int get(char* remotefilePath, char* localfilePath) = 0;	// return exit code
		virtual int put(char* remotefilePath, char* localfilePath) = 0;	// return exit code
		virtual int ls (char* localfilePath) = 0;	// return exit code
		virtual int del(char* remotefilePath) = 0;	// return exit code
};

#endif
