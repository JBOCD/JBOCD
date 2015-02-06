void Client::load_CDDriver(){
	sql::Statement* stmt;
	sql::ResultSet* res;
	int counter=0;
	// database get all list
	stmt = MySQL::getCon()->createStatement();
	int i=0,j=0;
	res = stmt->executeQuery("SELECT COUNT(id) FROM dropbox");
	if(res->next()){
		i=res->getInt(1);
		counter+=i;
		dropboxList = (CDDriver **) malloc(sizeof(CDDriver*) * (i+1));
		delete res;
	}

	res = stmt->executeQuery("SELECT COUNT(id) FROM googledrive");
	if(res->next()){
		j=res->getInt(1);
		counter+=i;
		googleDriveList = (CDDriver **) malloc(sizeof(CDDriver*) * (j+1));
		delete res;
	}

	if(i){
		i=0;
		res = stmt->executeQuery("SELECT `id`, `key` FROM dropbox");
		while(res->next()){
			dropboxList[i++] = new Dropbox(res->getString(2)->c_str(), res->getInt(1));
		}
		dropboxList[i]=0;
		delete res;
	}
	if(j){
		j=0;
		res = stmt->executeQuery("SELECT `id`, `key` FROM googledrive");
		while(res->next()){
			googleDriveList[j++] = new GoogleDrive(res->getString(2)->c_str(), res->getInt(1));
		}
		googleDriveList[j]=0;
		delete res;
	}
	// we assume that no extra cloud drive will add into the system
	// previous COUNT(id) should be equal to now COUNT(id)

}


void Client::doHandshake(){
//	if not using websocket, how to confirm message exact size in our protocol?
	recv(client_conf->connfd, buffer, WebSocket::MAX_PACKAGE_SIZE, 0);
	write(client_conf->connfd, buffer, WebSocket::getHandShakeResponse(buffer, buffer, &err));
}

Client::Client(Config* conf, struct client_info* conn_conf){
	this.conf = conf;
	this.conn_conf = conn_conf;

	int err;
	bool isCont = false;
	bool isEnd = false;
	CDDriver ** tmpCDD;

	unsigned char packageCode;

	buffer = (unsigned char*) malloc(WebSocket::MAX_PACKAGE_SIZE);
	recvLen=0;
	readLen=0;
	isCont = 0; // if recvLen != readLen, then isCont == true, it mean it continue to read;


	// file management
	char* remoteFileNameBuf = new char[1024];
	char* localFileNameBuf = new char[256];

	int* tmpFile = NULL;
	int tmpFilefd = 0;

	// for ls, get, put, delete
	unsigned char service;
	int serviceID;
	short pathLength;

	// for get, put
	int totalLen, fileRecvTotalLen, packageTotalLen, packageRecvLen, tmpLen;

	// for system result
	int result;

	// debug variable
	int msgNum = 0;

	Client::doHandshake();

	do{
		msgNum++;
		do{
			readLen = WebSocket::getMsg(client_conf->connfd, buffer, WebSocket::MAX_PACKAGE_SIZE, isCont, &recvLen, maskKey, &err);
			// err handling
			if(err & WebSocket::ERR_VER_MISMATCH) printf("WebSocket Version not match.\n");
			if(err & WebSocket::ERR_NOT_WEBSOCKET) printf("Connection is not WebScoket.\n");
			if(err & WebSocket::ERR_WRONG_WS_PROTOCOL) printf("WebSocket Protocol Error, Client Package have no mask.\n");
			if(err) break;

			// protocol part
			if(!isCont)	packageCode = *buffer;

			// recv part
			switch(packageCode){
				case 0x02: // ls acc req
					break;
				case 0x04: // ls dir req
					service = *(buffer+1);
					serviceID = Network::toInt(buffer+2);
					pathLength = Network::toShort(buffer+6);
					memcpy(remoteFileNameBuf, buffer+8, pathLength);
					remoteFileNameBuf[pathLength]=0;
					tmpFile = FileManager::newTemp();
					FileManager::getTempPath(tmpFile, localFileNameBuf);
					tmpCDD = service ? dropboxList : googleDriveList;
					i=0;
					while(tmpCDD[i] && !(tmpCDD[i]->isID(serviceID))) i++;
					if(tmpCDD[i]){
						printf("Calling API ... ");
						result = tmpCDD[i]->ls(remoteFileNameBuf, localFileNameBuf);
						printf("Done.");
					}
					break;
				case 0x20: // put req first recv part
					if(!isCont){
						tmpFile = FileManager::newTemp();
						service = *(buffer+1);
						serviceID = Network::toInt(buffer+2);
						pathLength = Network::toShort(buffer+6);
						memcpy(remoteFileNameBuf, buffer+8, pathLength);
						remoteFileNameBuf[pathLength]=0;
						FileManager::getTempPath(tmpFile, localFileNameBuf);
						if((tmpFilefd = open(localFileNameBuf,O_WRONLY)) != -1){
							totalLen = Network::toInt(buffer+8+pathLength);
							if(ftruncate(tmpFilefd,totalLen) != -1){
								packageTotalLen = Network::toInt(buffer+12+pathLength);
								tmpLen = readLen-16-pathLength;
								fileRecvTotalLen = 0;
								packageRecvLen = packageTotalLen-tmpLen;
								fileRecvTotalLen+=tmpLen;
								write(tmpFilefd,buffer-16-pathLength,tmpLen);
							}else{
								close(tmpFilefd);
								tmpFilefd = -1;
							}
						}
					}else if(tmpFilefd != -1){
						tmpLen = readLen;
						packageRecvLen -= readLen;
						fileRecvTotalLen+=tmpLen;
						write(tmpFilefd,buffer,tmpLen);
					}
					if(totalLen == fileRecvTotalLen){
						// file completed, start uploading
						close(tmpFilefd);
						tmpCDD = service ? dropboxList : googleDriveList;
						i=0;
						while(tmpCDD[i] && !(tmpCDD[i]->isID(serviceID))) i++;
						if(tmpCDD[i]){
							printf("Calling API ... ");
							result = tmpCDD[i]->put(remoteFileNameBuf, localFileNameBuf);
							printf("Done.");
						}
						FileManager::deleteTemp(tmpFile);
						tmpFile = 0;
					}
					break;
				case 0x21: // put req continue recv part
					// assume that ben is put all file in a one ws package

					break;
				case 0x22: // get req first send part
					service = *(buffer+1);
					serviceID = Network::toInt(buffer+2);
					pathLength = Network::toShort(buffer+6);
					memcpy(remoteFileNameBuf, buffer+8, pathLength);
					remoteFileNameBuf[pathLength]=0;
					tmpFile = FileManager::newTemp();
					FileManager::getTempPath(tmpFile, localFileNameBuf);
						// call api to get file
					tmpCDD = service ? dropboxList : googleDriveList;
					i=0;
					while(tmpCDD[i] && !(tmpCDD[i]->isID(serviceID))) i++;
					if(tmpCDD[i]){
						printf("Calling API ... ");
						result = tmpCDD[i]->get(remoteFileNameBuf, localFileNameBuf);
						printf("Done.");
					}
					break;
//				case 0x23: // get req continue send part
//					break;
				case 0x28: // delete file opearation
					service = *(buffer+1);
					serviceID = Network::toInt(buffer+2);
					pathLength = Network::toShort(buffer+6);
					memcpy(remoteFileNameBuf, buffer+8, pathLength);
					remoteFileNameBuf[pathLength]=0;
					tmpCDD = service ? dropboxList : googleDriveList;
					i=0;
					while(tmpCDD[i] && !(tmpCDD[i]->isID(serviceID))) i++;
					if(tmpCDD[i]){
						printf("Calling API ... ");
						result = tmpCDD[i]->del(remoteFileNameBuf);
						printf("Done.");
					}
					break;
				case 0x2A: // create dir
					break;
				case 0x88: // 10001000
					isEnd = true;
					break; 
				default:
/*
					// do nothing				
					for(int i=0;i<readLen;i++){
						putchar(buffer[i]);
					}
					memcpy(buffer, "hello world", 12);
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 12), 0);
*/
					;
			}

		}while(isCont=recvLen);

		if(err) break;
		// send part
		switch(packageCode){
			case 0x00: // 00000000
				// 4 byte int: send account id
				// 32byte string: token_hash sha256
				// reject first byte = 0x00
				// always accept
				*buffer = 0x01;
				*(buffer+1) = 0x00;
				send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 2), 0);
				break;
			case 0x02: // ls acc req
				break;
			case 0x04: // ls dir req
				*buffer = 0x04;
				if((tmpFilefd = open(localFileNameBuf,O_RDONLY)) != -1){
					Network::toBytes((int) lseek(tmpFilefd, 0L, SEEK_END)+1, buffer+1);
					lseek(tmpFilefd, 0L, SEEK_SET);
					readLen = read(tmpFilefd, buffer + 5, WebSocket::MAX_CONTENT_SIZE - 5);
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 5+readLen), 0);
					close(tmpFilefd);
				}else{
					// error handling
					Network::toBytes((int) 0, buffer+1);
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 5), 0);
				}
				FileManager::deleteTemp(tmpFile);
				tmpFile = 0;
				break;
			case 0x20: // put req first recv part
//			case 0x21: // put req continue recv part
				*buffer = packageCode;
				if(tmpFilefd != -1 && !packageRecvLen){
					*(buffer+1) = 0x00;
					Network::toBytes((int) packageTotalLen,buffer+2);
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 6), 0);
					fileRecvTotalLen += packageTotalLen;
					if(fileRecvTotalLen == totalLen){
						// end of read file
						// close file
						close(tmpFilefd);
						// call api to read
						FileManager::deleteTemp(tmpFile);
						tmpFile=0;
					}
				}else{
					if(tmpFilefd == -1){
						*(buffer+1) = 0xFF; // file not create Error
					}else if(packageRecvLen){
						*(buffer+1) = 0xFE; // file recv length not equal to sent length
						// err occur, close file and release
						close(tmpFilefd);
						tmpFilefd = -1;
					}
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 2), 0);
					FileManager::deleteTemp(tmpFile);
					tmpFile = 0;
				}

				break;
			case 0x22: // get req first send part
				*buffer = 0x22;
				if((tmpFilefd = open(localFileNameBuf,O_RDONLY)) != -1){
					Network::toBytes((int) (totalLen = lseek(tmpFilefd, 0L, SEEK_END)+1), buffer+1);
					lseek(tmpFilefd, 0L, SEEK_SET);
					
					Network::toBytes((int) (readLen = read(tmpFilefd, buffer + 9, WebSocket::MAX_CONTENT_SIZE - 9)), buffer+5);
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 9+readLen), 0);
					totalLen -= readLen;
					while(totalLen && readLen){
						*buffer = 0x23;
						Network::toBytes((int) lseek(tmpFilefd, 0L, SEEK_CUR), buffer+1);
						lseek(tmpFilefd, 0L, SEEK_SET);
						
						Network::toBytes((int) (readLen = read(tmpFilefd, buffer + 9, WebSocket::MAX_CONTENT_SIZE - 9)), buffer+5);
						send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 9+readLen), 0);
						totalLen -= readLen;
					}
					close(tmpFilefd);
					tmpFilefd = -1;
				}else{
					// error handling
					Network::toBytes((int) 0, buffer+1);
					Network::toBytes((int) 0, buffer+5);
					send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 9), 0);
				}
				FileManager::deleteTemp(tmpFile);
				tmpFile = 0;
				break;
			case 0x23: // get req continue send part
				break;
			case 0x28: // delete file opearation
				*buffer=0x28;
				*(buffer+1)=(char) result;
				send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 2), 0);
				break;
			case 0x2A: // create dir
				break;
			case 0x88: // 10001000
				isEnd = true;
				break; 
			default:
				for(int i=0;i<readLen;i++){
					putchar(buffer[i]);
				}
				memcpy(buffer, "hello world", 12);
				send(client_conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 12), 0);
		}
	}while(!isEnd);
	free(buffer);
	for(i=0;dropboxList[i];i++) delete dropboxList[i];
	for(i=0;googleDriveList[i];i++) delete googleDriveList[i];
	free(dropboxList);
	free(googleDriveList);
	delete remoteFileNameBuf;
	delete localFileNameBuf;
	close(client_conf->connfd);
	// end thread
	Thread::addDelQueue((struct thread_queue*) malloc(sizeof(struct thread_queue)),client_conf->thread, conf);
}

void Server::_client_close(int signum){
//	int status;
//	while(pid = waitpid(-1, &status, WNOHANG)){
//		conn_count--;
//	}
}
void thread_close(int signum){
	pid_t pid;
	int status;
	while(pid = waitpid(-1, &status, WNOHANG)){
		conn_count--;
	}
}
