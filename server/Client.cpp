Client::Client(struct client_info* conn_conf){
	this.conn_conf = conn_conf;
	inBuffer = (unsigned char*) malloc(WebSocket::MAX_PACKAGE_SIZE);
	outBuffer = (unsigned char*) malloc(WebSocket::MAX_PACKAGE_SIZE);
	res_root = res_last = NULL;

	Client::prepareStatement();
	Client::setAccount();
	Client::doHandshake();
	pthread_mutex_init(&res_mutex, NULL);
	pthread_mutex_init(&res_queue_mutex, NULL);
	pthread_mutex_lock(&res_mutex);
	pthread_create(&(responseThread), NULL, &Client::responseThread, NULL);
	Client::commandInterpreter();
}
void Client::loadServiceDrive(){
	sql::ResultSet* res;
	sql::Statement* stmt;
	
	unsigned int cdid;
	unsigned int lid;
	char* classname;
	char* tmpStr;
	char *error;
	int i=0, j;
	if(!cloudDrives){

		classname = (char*) MemManager::allocate(256);
		tmpStr = (char*) MemManager::allocate(512);
		stmt = MySQL::getCon()->createStatement();

		res = get_number_of_library->executeQuery();
		cd_handler = (struct clouddriver_handler_list*) MemManager::allocate(sizeof(struct clouddriver_handler_list) * (res->getInt("num_of_lib") + 1));
		memset(cd_handler, 0, sizeof(struct clouddriver_handler_list) * (res->getInt("num_of_lib") + 1));
		delete res;

		res = get_cloud_drive_list->executeQuery();
		cloudDrives = (CDDriver **) malloc(sizeof(CDDriver*) * (res->rowsCount() + 2)); // 1 is enough, 2 is safety
		while(res->next()){
			strcpy(classname, res->getString("classname")->c_str());
			cdid = res->getUInt("cdid");
			lid = res->getUInt("lid");
			for(j=0;!cd_handler[j]->lid && cd_handler[j]->lid != lid;j++);
			if(!cd_handler[j]->lid){
				sprintf(tmpStr, "/usr/local/lib/lib%s.so", classname); // path hardcode now
				cd_handler[j]->lid = lid;
				cd_handler[j]->handler = dlopen(tmpStr, RTLD_NOW | RTLD_LOCAL);
				if(!handle){
					fprintf(stderr, "Cannot load library. Path: \"%s\", Error: \"%s\"\n", tmpStr, dlerror());
					cd_handler[j]->lid = 0;
					continue;
				}
				*(void **)(&(cd_handler[j]->newCDDriver))=dlsym(handle,"createObject");
				if((error=dlerror())!=NULL){
					fprintf(stderr, "Cannot find function \"CDDriver* createObject(const char*, int)\". Class: \"%s\", Error: \"%s\"\n", classname, error);
					dlclose(cd_handler[j]->handler);
					cd_handler[j]->lid = 0;
					continue;
				}
			}
			delete res;
			sprintf(tmpStr, "SELECT `id`, `key` FROM `%s` WHERE `id`=%u", classname, cdid);
			res = stmt->executeQuery(tmpStr);
			// http://www.linuxjournal.com/article/3687?page=0,0
			cloudDrives[i] = (*cd_handler[j]->newCDDriver)(res->getString("key"), res->getUInt("id"));
			i++;
		}
		cloudDrives[i]=NULL;
		delete res;
		delete stmt;
		MemManager::free(classname);
		MemManager::free(tmpStr);
	}
}

void Client::prepareStatement(){
//	used in 0x00, 0x01
	check_token = MySQL::getCon()->prepareStatement("SELECT * FROM `token` WHERE `token`=? AND `id`=?");

//	used in 0x02, loadServiceDrive
	get_cloud_drive_list = MySQL::getCon()->prepareStatement("SELECT `a`.`cdid` as `cdid`, `a`.`lid`, `b`.`dir` as `classname` FROM `clouddrive` as `a`, `libraries` as `b` WHERE `a`.`uid`=? AND `a`.`id`=`b`.`lid`");
	get_number_of_library = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT COUNT(`lid`) FROM `clouddrive` WHERE `uid`=? GROUP BY `lid`),0) as `num_of_lib`");

//	used in 0x03
	get_logical_drive_list = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldriveinfo` WHERE `uid`=?");

//	used in 0x04
	get_list = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `parentid`=?");

//	used in 0x04, 0x20, 0x22
	get_logical_drive = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldriveinfo` WHERE `uid`=?");
	get_cloud_drive_by_ldid = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldrivecontainer` WHERE `ldid`=?");

//	used in 0x21
	insert_chunk = MySQL::getCon()->prepareStatement("INSERT INTO `filechunk` (`ldid`, `cdid`, `fileid`, `seqnum`, `chunk_name`, `size`) VALUE (?,?,?,?,?,?)");

//	used in 0x22
	get_file_chunk = MySQL::getCon()->prepareStatement("SELECT * FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");

//	used in 0x20, 0x2A
	get_next_fileid = MySQL::getCon()->prepareStatement("SELECT IFNULL((SELECT MAX(fileid)+1 FROM `directory` WHERE `ldid`=1 GROUP BY ldid), 1) as `fileid`");
	create_file = MySQL::getCon()->prepareStatement("INSERT INTO `directory` (`ldid`, `parentid`, `fileid`, `name`, `size`) VALUE (?,?,?,?,?)");
}

void Client::setAccount(){
	check_user_logical_drive->setInt(2, account_id);
	get_cloud_drive_list->setInt(1, account_id);
	get_number_of_library->setInt(1, account_id);
	get_user_logical_drive->setInt(1, account_id);
}

void Client::doHandshake(){
//	if not using websocket, how to confirm message exact size in our protocol?
	recv(conn_conf->connfd, inBuffer, WebSocket::MAX_PACKAGE_SIZE, 0);
	write(conn_conf->connfd, outBuffer, WebSocket::getHandShakeResponse(inBuffer, outBuffer, &err));
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
		// should check login then do other step
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

// !important, how about login not ok? pointer is not set properly

// 0x00, 0x01 //done
void Client::readLogin(){
	struct client_list* info = (struct client_list*) MemManager::allocate(sizeof(struct client_list));
	char* token = inBuffer+6;
	token[32]=0;
	info->operationID = *(inBuffer+1);
	accountID = Network::toInt(inBuffer+2);
	check_token->setString(1, token);
	check_token->setUInt(2, accountID);
	res = check_token->executeQuery();
	if(res->rowsCount() == 1){
		Client::setAccount();
		Client::addResponseQueue(0x01, info);
	}else{
		accountID=0;
		Client::addResponseQueue(0x00, info);
	}
	delete res;
}
// 0x02 //done
void Client::readGetService(){
	struct client_clouddrive_root* info = (struct client_clouddrive_root*) MemManager::allocate(sizeof(struct client_clouddrive_root));
	Client::loadServiceDrive();
	info->operationID = *(inBuffer+1);
	info->root = cloudDrives;
	Client::addResponseQueue(0x02, info);
}
// 0x03 //done
void Client::readGetDrive(){
	struct client_logical_drive_root* info = (struct client_logical_drive_root*) MemManager::allocate(sizeof(struct client_logical_drive_root));
	struct client_logical_drive* ld_last;
	struct client_clouddrive* cd_last;
	sql::ResultSet *res;
	sql::ResultSet *res1;
	char* tmpstr;

	info->operationID = *(inBuffer+1);
	info->root = NULL;
	check_user_logical_drive->setInt(1, logicalDriveID);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		delete res;
		res = get_logical_drive->executeQuery();
		while(res->next()){
			if(info->root){
				ld_last = ( ld_last->next = (struct client_logical_drive*) MemManager::allocate(sizeof(struct client_logical_drive)) );
			}else{
				ld_last = ( info->root = (struct client_logical_drive*) MemManager::allocate(sizeof(struct client_logical_drive)) );
			}
			tmpstr = (char*)MemManager::allocate(strlen(res->getString('name')->c_str()));
			strcpy(tmpstr, res->getString('name')->c_str());

			ld_last->ldid = res->getUInt('ldid');
			ld_last->algoid = res->getUInt('algoid');
			ld_last->name = tmpstr;
			ld_last->size = res->getUInt64('size');
			ld_last->root = NULL;
			ld_last->next = NULL;
			get_cloud_drive_by_ldid->setUInt(1, ld_last->ldid);
			res1 = get_cloud_drive_by_ldid->executeQuery();
			while(res1->next()){
				if(info->root){
					cd_last = ( cd_last->next = (struct client_clouddrive*) MemManager::allocate(sizeof(struct client_clouddrive)) );
				}else{
					cd_last = ( ld_last->root = (struct client_clouddrive*) MemManager::allocate(sizeof(struct client_clouddrive)) );
				}
				cd_last->cdid = res1->getUInt('cdid');
				cd_last->size = res1->getUInt64('size');
				cd_last->next = NULL;
			}
			delete res1;

		}

	}
	delete res;
	Client::addResponseQueue(0x03, info);
}
// 0x04 //done
void Client::readList(){
	struct client_list_root* info = (struct client_list_root*) MemManager::allocate(sizeof(struct client_list_root));
	struct client_list* tmp;
	unsigned int ldid = Network::toInt(inBuffer+2);
	unsigned long long parentid = Network::toLongLong(inBuffer+6);
	const char* tmpstr;
	sql::ResultSet *res;
	info->operationID = *(inBuffer+1);

	check_user_logical_drive->setUInt(1, ldid);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		delete res;
		get_list->setUInt(1, ldid);
		get_list->setUInt64(2, parentid);
		res = get_list->executeQuery();
		info->numberOfFile = res->rowsCount();
		while(res->next()){
			if(info->root){
				tmp->next = (struct client_list*) MemManager::allocate(sizeof(struct client_list));
				tmp = tmp->next;
			}else{
				info->root = (tmp = (struct client_list*) MemManager::allocate(sizeof(struct client_list)));
			}
			tmp->fid = res->getUInt64('fileid');
			tmp->fileSize = res->getUInt64('size');
			tmp->next = NULL;

			tmpstr = res->getString('name')->c_str();
			tmp->name = (char*) MemManager::allocate(strlen(tmpstr)+1);
			strcpy(tmp->name, tmpstr);
		}
	}
	delete res;

	Client::addResponseQueue(0x04, (void*)info);
}
// 0x20 //done
void Client::readCreateFile(){
	struct client_make_file* info = (struct client_make_file*) MemManager::allocate(sizeof(struct client_make_file));
	unsigned int logicalDriveID = Network::toInt(inBuffer+2);
	unsigned long long parentID = Network::toLongLong(inBuffer+6);
	unsigned long long fileSize = Network::toLongLong(inBuffer+14);
	char* name = (char*) Network::toChars(inBuffer+22);
	sql::ResultSet *res;

	info->operationID = *(inBuffer+1);

	check_user_logical_drive->setInt(1, logicalDriveID);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		delete res;
		get_next_fileid->setInt(1, logicalDriveID);
		res = get_next_fileid->executeQuery();
		while(res->next()){
			info->fid=res->getUInt64("fileid");
			// `ldid`, `parentid`, `fileid`, `name`, `size`
			create_file->setUInt(1, logicalDriveID);
			create_file->setUInt64(2, parentID);
			create_file->setUInt64(3, info->fid);
			create_file->setString(4, name);
			create_file->setUInt64(5, fileSize);
			if(!create_file->executeUpdate()){
				info->fid = 0;
			}
			break;
		}
	}
	delete res;
	Client::addResponseQueue(0x20, (void*) info));
}
// 0x21 //done
void Client::readSaveFile(){

	struct client_save_file* info = (struct client_save_file*) MemManager::allocate(sizeof(struct client_save_file));
	unsigned short bufShift = 0;
	unsigned long long maxSaveSize;
	int fd;
	info->isInsertOK = 0;
	info->operationID = *(inBuffer +1);
	info->ldid = Network::toInt(inBuffer + 2);
	info->cdid = Network::toInt(inBuffer + 6);
	info->fid = Network::toLongLong(inBuffer + 10);
	info->seqnum = Network::toLongLong(inBuffer + 18);
	info->remoteName = (char*) Network::toChars(inBuffer + 26);
	bufShift = 30 + Network::toShort(inBuffer + 26);
	maxSaveSize = (info->chunkSize = Network::toInt(inBuffer + bufShift - 4));

	info->tmpFile = FileManager::newTemp(info->chunkSize);

	fd = FileManager::open(info->tmpFile, 'w');
	write(fd, inBuffer + bufShift, readLen - bufShift);
	maxSaveSize -= readLen - bufShift;
	// check whether this call will have some security problem
	while(recvLen && maxSaveSize > 0 && maxSaveSize < info->chunkSize){
		readLen = WebSocket::getMsg(conn_conf->connfd, inBuffer, WebSocket::MAX_PACKAGE_SIZE > recvLen ? recvLen : WebSocket::MAX_PACKAGE_SIZE, true, &recvLen, maskKey, &err);
		write(fd, inBuffer, maxSaveSize > readLen ? readLen : maxSaveSize);
		maxSaveSize -= readLen;
	}
	close(fd);

	while(recvLen > 0){
		WebSocket::getMsg(conn_conf->connfd, inBuffer, WebSocket::MAX_PACKAGE_SIZE > recvLen ? recvLen : WebSocket::MAX_PACKAGE_SIZE, true, &recvLen, maskKey, &err);
	}
	isCont = false;

	check_user_logical_drive->setInt(1, logicalDriveID);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		insert_chunk->setUInt(1,info->ldid);
		insert_chunk->setUInt(2,info->cdid);
		insert_chunk->setUInt64(3,info->fid);
		insert_chunk->setUInt64(4,info->seqnum);
		insert_chunk->setString(5,info->remoteName);
		insert_chunk->setUInt(6,info->chunkSize);
		info->isInsertOK = insert_chunk->executeUpdate();
	}
	delete res;
	Thread::create(&Client::processSaveFile, (void*) info));
}
// 0x22 //done
void Client::readGetFile(){
	struct client_read_file* info;
	unsigned int logicalDriveID = Network::toInt(inBuffer + 2);
	unsigned long long fileID = Network::toLongLong(inBuffer + 6);
	const char* tmpstr;
	sql::ResultSet *res;

	check_user_logical_drive->setInt(1, logicalDriveID);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		delete res;

		get_file_chunk->setUInt(1, logicalDriveID); // get_file_chunk is undefine
		get_file_chunk->setUInt64(2, fileID);
		res = get_file_chunk->executeQuery();
		while(res->next()){
			info = (struct client_read_file*) MemManager::allocate(sizeof(struct client_read_file));
			info->operationID = *(inBuffer + 1);
			info->ldid = logicalDriveID;
			info->cdid = res->getInt("cdid");
			info->fileid = fileID;
			info->seqnum = res->getInt("seqnum");
			tmpstr = res->getString("chunk_name")->c_str();
			info->chunkName = (char*) MemManager::allocate(strlen(tmpstr)+1);
			strcpy(info->chunkName, tmpstr);
			info->chunkSize = res->getInt("size");
			Thread::create(&Client::processGetFile, (void*) info));
		}
	}
	delete res;
}
// 0x28 //done
void Client::readDelFile(){
	struct client_del_file* info = (struct client_del_file*) MemManager::allocate(sizeof(struct client_del_file));
	info->operationID = *(inBuffer+1);
	info->ldid = Network::toInt(inBuffer + 2);
	info->fileid = Network::toInt(inBuffer + 6);

	Thread::create(&Client::processDelFile, (void*) info));
}

/* Process Command */
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
