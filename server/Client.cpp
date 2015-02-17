Client::Client(Config* conf, struct client_info* conn_conf){
	this.conf = conf;
	this.conn_conf = conn_conf;
	inBuffer = (unsigned char*) malloc(WebSocket::MAX_PACKAGE_SIZE);
	outBuffer = (unsigned char*) malloc(WebSocket::MAX_PACKAGE_SIZE);
	res_root = res_last = NULL;


	Client::doHandshake();
	pthread_mutex_init(&res_mutex, NULL);
	pthread_mutex_init(&res_queue_mutex, NULL);
	pthread_mutex_lock(&res_mutex);
	pthread_create(&(responseThread), NULL, &Client::responseThread, NULL);
	Client::commandInterpreter();
}
void Client::loadServiceDrive(){
	sql::Statement* stmt;
	sql::ResultSet* res;
	int counter=0;
	char *sqlsmt = (char*)malloc(90);
	// database get all list
	strcpy(sqlsmt, "SELECT COUNT(id)   FROM ");
	stmt = MySQL::getCon()->createStatement();
	int i=0,j=0;

	strcpy(sqlsmt+24, "dropbox");
	res = stmt->executeQuery(sqlsmt);
	if(res->next()){
		i=res->getInt(1);
		counter+=i;
		dropboxList = (CDDriver **) malloc(sizeof(CDDriver*) * (i+1));
		delete res;
	}
	strcpy(sqlsmt+24, "googledrive");
	memcpy(sqlsmt+7, "COUNT(id)  ",11);
	res = stmt->executeQuery(sqlsmt);
	if(res->next()){
		j=res->getInt(1);
		counter+=i;
		googleDriveList = (CDDriver **) malloc(sizeof(CDDriver*) * (j+1));
		delete res;
	}
	cloudDrives = (CDDriver **) malloc(sizeof(CDDriver*) * (i+j+1));

	strcpy(sqlsmt+24, "dropbox");
	memcpy(sqlsmt+7, "`id`, `key`",11);
	if(i){
		i=0;
		res = stmt->executeQuery("SELECT `id`, `key` FROM dropbox");
		for(i=0;res->next();i++){
			cloudDrives[i] = (dropboxList[i] = new Dropbox(res->getString(2)->c_str(), res->getInt(1)));
		}
		cloudDrives[i] = dropboxList[i] = 0;
		delete res;
	}

	if(j){
		j=0;
		strcpy(sqlsmt+24, "googledrive");
		memcpy(sqlsmt+7, "`id`, `key`",11);
		res = stmt->executeQuery(sqlsmt);
		for(j=0;res->next();j++){
			cloudDrives[i+j] = (googleDriveList[j] = new GoogleDrive(res->getString(2)->c_str(), res->getInt(1)));
		}
		cloudDrives[i+j] = googleDriveList[j] = 0;
		delete res;
	}
	// we assume that no extra cloud drive will add into the system
	// previous COUNT(id) should be equal to now COUNT(id)

}


void Client::doHandshake(){
//	if not using websocket, how to confirm message exact size in our protocol?
	recv(conn_conf->connfd, buffer, WebSocket::MAX_PACKAGE_SIZE, 0);
	write(conn_conf->connfd, buffer, WebSocket::getHandShakeResponse(buffer, buffer, &err));
}

void Client::commandInterpreter(){

	unsigned char packageCode;

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


	recvLen=0;
	readLen=0;
	isCont = 0; // if recvLen != readLen, then isCont == true, it mean it continue to read;
	isCont = isEnd = false;


	do{
		msgNum++;

		readLen = WebSocket::getMsg(conn_conf->connfd, inBuffer, isCont && (WebSocket::MAX_PACKAGE_SIZE > recvLen)? recvLen : WebSocket::MAX_PACKAGE_SIZE, isCont, &recvLen, maskKey, &err);
		// err handling
		if(err & WebSocket::ERR_VER_MISMATCH) printf("WebSocket Version not match.\n");
		if(err & WebSocket::ERR_NOT_WEBSOCKET) printf("Connection is not WebScoket.\n");
		if(err & WebSocket::ERR_WRONG_WS_PROTOCOL) printf("WebSocket Protocol Error, Client Package have no mask.\n");
		if(err) break;

		// protocol part
		if(!isCont)	packageCode = *inBuffer;

		// recv part
		switch(packageCode){
			case 0x00: // ls acc req
				Client::processLogin();
				break;
			case 0x02:
				Client::readGetService();
				break;
			case 0x03:
				Client::readGetDrive();
				break;
			case 0x04:
				Client::readList();
				break;
/* // this is list cloud drive directory
			case 0x04: // ls dir req
				service = *(inBuffer+1);
				serviceID = Network::toInt(inBuffer+2);
				pathLength = Network::toShort(inBuffer+6);
				memcpy(remoteFileNameBuf, inBuffer+8, pathLength);
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
*/
			case 0x20:
				Client::readCreateFile();
				// no break;
			case 0x21:
				Client::readSaveFile();
				break;
/*
			case 0x20: // put req first recv part
				if(!isCont){
					tmpFile = FileManager::newTemp();
					service = *(inBuffer+1);
					serviceID = Network::toInt(inBuffer+2);
					pathLength = Network::toShort(inBuffer+6);
					memcpy(remoteFileNameBuf, inBuffer+8, pathLength);
					remoteFileNameBuf[pathLength]=0;
					FileManager::getTempPath(tmpFile, localFileNameBuf);
					if((tmpFilefd = open(localFileNameBuf,O_WRONLY)) != -1){
						totalLen = Network::toInt(inBuffer+8+pathLength);
						if(ftruncate(tmpFilefd,totalLen) != -1){
							packageTotalLen = Network::toInt(inBuffer+12+pathLength);
							tmpLen = readLen-16-pathLength;
							fileRecvTotalLen = 0;
							packageRecvLen = packageTotalLen-tmpLen;
							fileRecvTotalLen+=tmpLen;
							write(tmpFilefd,inBuffer-16-pathLength,tmpLen);
						}else{
							close(tmpFilefd);
							tmpFilefd = -1;
						}
					}
				}else if(tmpFilefd != -1){
					tmpLen = readLen;
					packageRecvLen -= readLen;
					fileRecvTotalLen+=tmpLen;
					write(tmpFilefd,inBuffer,tmpLen);
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
*/
			case 0x22:
				Client::readGetFile();
				break;
/*
			case 0x22: // get req first send part
				service = *(inBuffer+1);
				serviceID = Network::toInt(inBuffer+2);
				pathLength = Network::toShort(inBuffer+6);
				memcpy(remoteFileNameBuf, inBuffer+8, pathLength);
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
				case 0x23: // get req continue send part
					break;
*/
			case 0x28:
				Client::readDelFile();
				break;
			case 0x2A:
				Client::readMkdir();
				break;
/*
			case 0x28: // delete file opearation
				service = *(inBuffer+1);
				serviceID = Network::toInt(inBuffer+2);
				pathLength = Network::toShort(inBuffer+6);
				memcpy(remoteFileNameBuf, inBuffer+8, pathLength);
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
*/
			case 0x88: // 10001000
				Client::addResponseQueue(0x88, NULL);
				isEnd = true;
				break; 
		}

	}while(!isEnd);
	free(buffer);
	for(i=0;dropboxList[i];i++) delete dropboxList[i];
	for(i=0;googleDriveList[i];i++) delete googleDriveList[i];
	free(dropboxList);
	free(googleDriveList);
	delete remoteFileNameBuf;
	delete localFileNameBuf;
	close(conn_conf->connfd);
	// end thread
//	Thread::addDelQueue((struct thread_queue*) malloc(sizeof(struct thread_queue)),conn_conf->thread, conn_conf);
}

/* resposne function */
void Client::addResponseQueue(unsigned char command, void* info){
	struct client_response* tmp = (struct client_response*) MemManager::allocate(sizeof(struct client_response));
	tmp->command = command;
	tmp->info = info;
	tmp->next = NULL;
	pthread_mutex_lock(&res_queue_mutex);
	if(res_root == NULL){
		res_root = tmp;
	}else{
		res_last->next = tmp;
	}
	res_last = tmp;
	pthread_mutex_unlock(&res_queue_mutex);
	pthread_mutex_unlock(&res_mutex);
}

void Client::responseThread(){
	struct client_response* tmp;
	do{
		pthread_mutex_lock(&res_mutex);
		do{
			pthread_mutex_lock(&res_queue_mutex);
			tmp = res_root;
			res_root = tmp->next;
			if(tmp == res_last){
				res_last = res_last->next; // it's always NULL
			}
			pthread_mutex_unlock(&res_queue_mutex);
			if(tmp != NULL){
				switch(tmp->command){
					case 0x00:
					case 0x01:
						Client::sendLogin(tmp->command);
						break;
					case 0x02:
					case 0x03:
					case 0x04:

						break;
					case 0x20:
						break;
				}
			}
			MemManager::free(tmp);
		}while(res_root);
	}while(!isEnd);
}
/*  backup code
		if(err) break;
		// send part
		switch(packageCode){
			case 0x00: // 00000000
				// 4 byte int: send account id
				// 32byte string: token_hash sha256
				// reject first byte = 0x00
				// always accept
				*outBuffer = 0x01;
				*(outBuffer+1) = 0x00;
				send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 2), 0);
				break;
			case 0x02: // ls acc req
				break;
			case 0x04: // ls dir req
				*outBuffer = 0x04;
				if((tmpFilefd = open(localFileNameBuf,O_RDONLY)) != -1){
					Network::toBytes((int) lseek(tmpFilefd, 0L, SEEK_END), outBuffer+1);
					lseek(tmpFilefd, 0L, SEEK_SET);
					readLen = read(tmpFilefd, outBuffer + 5, WebSocket::MAX_CONTENT_SIZE - 5);
					send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 5+readLen), 0);
					close(tmpFilefd);
				}else{
					// error handling
					Network::toBytes((int) 0, outBuffer+1);
					send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 5), 0);
				}
				FileManager::deleteTemp(tmpFile);
				tmpFile = 0;
				break;
			case 0x20: // put req first recv part
		//	case 0x21: // put req continue recv part
				if(totalLen == fileRecvTotalLen){
					*outBuffer = packageCode;
					if(tmpFilefd != -1 && !packageRecvLen){
						*(outBuffer+1) = 0x00;
						Network::toBytes((int) packageTotalLen,outBuffer+2);
						send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 6), 0);
					}else{
						if(tmpFilefd == -1){
							*(outBuffer+1) = 0xFF; // file not create Error
						}else if(packageRecvLen){
							*(outBuffer+1) = 0xFE; // file recv length not equal to sent length
							// err occur, close file and release
							close(tmpFilefd);
							tmpFilefd = -1;
						}
						send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 2), 0);
						FileManager::deleteTemp(tmpFile);
						tmpFile = 0;
					}
				}
				break;
			case 0x22: // get req first send part
				*outBuffer = 0x22;
				if((tmpFilefd = open(localFileNameBuf,O_RDONLY)) != -1){
					Network::toBytes((int) (totalLen = lseek(tmpFilefd, 0L, SEEK_END)+1), outBuffer+1);
					
					Network::toBytes((int) (readLen = read(tmpFilefd, outBuffer + 9, WebSocket::MAX_CONTENT_SIZE - 9)), outBuffer+5);
					send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 9+readLen), 0);
					totalLen -= readLen;
					while(totalLen && readLen){
						*outBuffer = 0x23;
						Network::toBytes((int) lseek(tmpFilefd, 0L, SEEK_CUR), outBuffer+1);
						lseek(tmpFilefd, 0L, SEEK_SET);
						
						Network::toBytes((int) (readLen = read(tmpFilefd, outBuffer + 9, WebSocket::MAX_CONTENT_SIZE - 9)), outBuffer+5);
						send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 9+readLen), 0);
						totalLen -= readLen;
					}
					fileRecvTotalLen = totalLen;
					close(tmpFilefd);
					tmpFilefd = -1;
				}else{
					// error handling
					Network::toBytes((int) 0, outBuffer+1);
					Network::toBytes((int) 0, outBuffer+5);
					send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 9), 0);
				}
				FileManager::deleteTemp(tmpFile);
				tmpFile = 0;
				break;
			case 0x23: // get req continue send part
				break;
			case 0x28: // delete file opearation
				*outBuffer=0x28;
				*(outBuffer+1)=(char) result;
				send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 2), 0);
				break;
			case 0x2A: // create dir
				break;
			case 0x88: // 10001000
				isEnd = true;
				break; 
			default:
		//		for(int i=0;i<readLen;i++){
		//			putchar(outBuffer[i]);
		//		}
		//		memcpy(outBuffer, "hello world", 12);
		//		send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 12), 0);
				;
		}
*/

/* Read Command */
// 0x00, 0x01 directly go process, no thread accepted
// 0x02
void Client::readGetService(){
	Thread::create(&Client::processGetService, (void *) *(inBuffer+1)));
}
// 0x03
void Client::readGetDrive(){
	Thread::create(&Client::processGetDrive, (void *) *(inBuffer+1));
}
// 0x04
void Client::readList(){
	struct client_list* info = (struct client_list*) MemManager::allocate(sizeof(struct client_list));
	info->operationID = *(inBuffer+1);
	info->did = Network::toInt(inBuffer+2);
	info->dirPath = (char*) MemManager::toChars(inBuffer+6);
	Thread::create(&Client::processList, (void*) info));
}
// 0x20 (first director)
void Client::readUploadFile(){
	Client::readCreateFile();
	Client::readSaveFile();
}
// 0x20, 0x2A
void Client::readCreateFile(){
	unsigned int fid; // ????? 8 bytes?
	unsigned int pid = Network::toInt(inBuffer+2);
	unsigned long long fileSize = Network::toLongLong(inBuffer+6);
	char* name = (char*) Network::toChars(inBuffer+14);

	// database operation
	fid = 1; // result

	// update inBuffer
	Network::toBytes((int) 12+Network::toShort(inBuffer+14),inBuffer+2);
	Network::toBytes(fid, inBuffer + Network::toInt(inBuffer+2));
}
// 0x21, 0x20 (remain process)
void Client::readSaveFile(){
	struct client_save_file* info = (struct client_save_file*) MemManager::allocate(sizeof(struct client_save_file));
	unsigned int* file_id = FileManager::newTemp();
	int bufShift = 0;
	int fd;
	info->operationID = *(inBuffer +1);
	bufShift = info->operationID == 0x20 ? Network::toInt(bufShift+2) : 2;

	info->fid = Network::toInt(inBuffer + bufShift);
	info->did = Network::toInt(inBuffer + bufShift + 4);
	info->sid = Network::toInt(inBuffer + bufShift + 8);
	info->seqnum = Network::toInt(inBuffer + bufShift + 12);
	info->remoteName = (char*) MemManager::toChars(inBuffer + bufShift + 16);
	bufShift = bufShift + 18 + Network::toShort(inBuffer + bufShift + 16);
	info->chunkSize = Network::toInt(inBuffer + bufShift);
	info->tmpFile = FileManager::newTemp(info->chunkSize);

	bufShift = bufShift + 4;
	fd = FileManager::open(info->tmpFile, 'w');
	do{

	}while(recvLen != readLen);
	Thread::create(&Client::processSaveFile, (void*) info));
}
// 0x22
void Client::readGetFile(){
	struct response_thread* thread;
	int numberOfFileChunk=0;

	for(int i=0;i<numberOfFileChunk;i++){
		thread = MemManager::allocate(sizeof(struct response_thread));
		pthread_create(&(thread->tid), NULL, &Client::processGetFile, (void*) thread));
	}
}
// 0x28
void Client::readDelFile(){
	struct response_thread* thread = MemManager::allocate(sizeof(struct response_thread));
	unsigned int* file_id = FileManager::newTemp();

	pthread_create(&(thread->tid), NULL, &Client::processDelFile, (void*) thread));
}
// 0x2A
void Client::readMkdir(){
	Client::readCreateFile();

	Client::addResponseQueue(0x2A, NULL);
}

/* Process Command */
// 0x00, 0x01
void Client::processLogin(){
	int* accountID = Network::toInt(inBuffer+1);
	char* token = inBuffer+5;
	*(token+32)=0;
	Client::addResponseQueue(0x00 || 0x01, NULL);
}
// 0x02
void Client::processGetService(void *arg){
	pthread_t tid = ((struct response_thread*) arg)->tid;
	MemManager::free(arg);

	Client::addResponseQueue(0x02, NULL);
	Thread::addDelQueue(tid);
}
// 0x03
void Client::processGetDrive(void *arg){
	pthread_t tid = ((struct response_thread*) arg)->tid;
	MemManager::free(arg);


	Client::addResponseQueue(0x03, NULL);
	Thread::addDelQueue(tid);
}
// 0x04
void Client::processList(void *arg){
	struct response_thread* thread = (struct response_thread*) arg;
	pthread_t tid = thread->tid;


	Client::addResponseQueue(0x04, NULL);
	MemManager::free(thread);
	Thread::addDelQueue(tid);
}
// 0x20, 0x21
void Client::processSaveFile(void *arg){
	struct response_thread* thread = (struct response_thread*) arg;
	pthread_t tid = thread->tid;


	Client::addResponseQueue(0x21, NULL);
	MemManager::free(thread);
	Thread::addDelQueue(tid);
}
// 0x22, 0x23
void Client::processGetFile(void *arg){
	struct response_thread* thread = (struct response_thread*) arg;
	pthread_t tid = thread->tid;


	Client::addResponseQueue(0x22, NULL);
	MemManager::free(thread);
	Thread::addDelQueue(tid);
}
// 0x28
void Client::processDelFile(void *arg){
	struct response_thread* thread = (struct response_thread*) arg;
	pthread_t tid = thread->tid;


	Client::addResponseQueue(0x28, NULL);
	MemManager::free(thread);
	Thread::addDelQueue(tid);
}

/* Send Result */
// 0x01
void Client::sendLogin(unsigned char command){
	*outBuffer=command;
	send(conn_conf->connfd, outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 1), 0);
				
}
