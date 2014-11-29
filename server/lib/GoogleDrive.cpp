GoogleDrive::GoogleDrive(char* accessToken){
	this->accessToken = accessToken;
	tmpStr = (char*) malloc(4096); // 4KB block
}
int GoogleDrive::get(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s %s %s %s",
		"/home/pcl1401/dev/modular/get.py",
		accessToken,
		remotefilePath,
		localfilePath
	);
	return system(tmpStr);
}
int GoogleDrive::put(char* remotefilePath, char* localfilePath){
	sprintf(tmpStr, "%s %s %s %s",
		"/home/pcl1401/dev/modular/put.py",
		accessToken,
		localfilePath,
		remotefilePath
	);
	return system(tmpStr);
}
int GoogleDrive::ls(char* localfilePath){
	sprintf(tmpStr, "%s %s %s",
		"/home/pcl1401/dev/modular/list.py",
		accessToken,
		localfilePath
	);
	return system(tmpStr);
}
int GoogleDrive::del(char* remotefilePath){
	sprintf(tmpStr, "%s %s %s",
		"/home/pcl1401/dev/modular/list.py",
		accessToken,
		remotefilePath
	);
	return system(tmpStr);
}

GoogleDrive::~GoogleDrive(){
	if(tmpStr){
		free(tmpStr);
		tmpStr = 0;
	}
}