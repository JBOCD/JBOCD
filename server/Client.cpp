Client::Client(){
	struct client_thread_director* info = (struct client_thread_director*) MemManager::allocate(sizeof(struct client_thread_director));
	info->object = this;
	info->fptr = &Client::responseThread;
	inBuffer = (unsigned char*) malloc(WebSocket::MAX_BUFFER_SIZE); // +8 for decode
	outBuffer = (unsigned char*) malloc(WebSocket::MAX_BUFFER_SIZE);
	res_root = res_last = NULL;
	account_id = 0;

	Client::prepareStatement();
	Client::updatePrepareStatementAccount();
	Client::doHandshake();

	pthread_mutex_init(&res_mutex, NULL);
	pthread_mutex_init(&res_queue_mutex, NULL);
	pthread_mutex_init(&client_end_mutex, NULL);
	pthread_mutex_lock(&res_mutex);
	pthread_create(&(responseThread_tid), NULL, &Client::_thread_redirector, info);
	Client::commandInterpreter();
}
void Client::loadCloudDrive(){
	sql::PreparedStatement* get_cloud_drive_list = MySQL::getCon()->prepareStatement("SELECT `a`.`cdid` as `cdid`, `a`.`lid`, `b`.`dir` as `classname` FROM `clouddrive` as `a`, `libraries` as `b` WHERE `a`.`uid`=? AND `a`.`lid`=`b`.`id`");
	sql::PreparedStatement* get_number_of_library = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT COUNT(`lid`) FROM `clouddrive` WHERE `uid`=? GROUP BY `lid`),0) as `num_of_lib`");
	sql::ResultSet* res;
	sql::ResultSet* res1;
	sql::Statement* stmt;
	
	unsigned int cdid;
	unsigned int lid;
	char* classname = (char*) MemManager::allocate(256);
	char* tmpStr = (char*) MemManager::allocate(512);
	char *error;
	int j;

	stmt = MySQL::getCon()->createStatement();

	get_number_of_library->setInt(1, account_id);
	res = get_number_of_library->executeQuery();

	if(res->next()){
		cd_handler = (struct clouddriver_handler_list*) MemManager::allocate(sizeof(struct clouddriver_handler_list) * (res->getInt("num_of_lib") + 1));
		memset(cd_handler, 0, sizeof(struct clouddriver_handler_list) * (res->getInt("num_of_lib") + 1));
		delete res;

		get_cloud_drive_list->setInt(1, account_id);
		res = get_cloud_drive_list->executeQuery();

		cd_root = (struct client_clouddrive_root*) MemManager::allocate(sizeof(struct client_clouddrive_root));
		cd_root->root = (CDDriver **) malloc(sizeof(CDDriver*) * (res->rowsCount() + 2)); // 1 is enough, 2 is safety
		for(cd_root->numOfCloudDrives=0; res->next(); cd_root->numOfCloudDrives++){
			strcpy(classname, res->getString("classname")->c_str());
			cdid = res->getUInt("cdid");
			lid = res->getUInt("lid");
			for(j=0;!cd_handler[j].lid && cd_handler[j].lid != lid;j++);
			if(!cd_handler[j].lid){
				sprintf(tmpStr, "/usr/local/lib/lib%s.so", classname); // path hardcode now
				cd_handler[j].lid = lid;
				cd_handler[j].handler = dlopen(tmpStr, RTLD_NOW | RTLD_LOCAL);
				if(!cd_handler[j].handler){
					fprintf(stderr, "Cannot load library. Path: \"%s\", Error: \"%s\"\n", tmpStr, dlerror());
					cd_handler[j].lid = 0;
					continue;
				}
				*(void **)(&(cd_handler[j].newCDDriver))=dlsym(cd_handler[j].handler,"createObject");
				if((error=dlerror())!=NULL){
					fprintf(stderr, "Cannot find function \"CDDriver* createObject(const char*, int)\". Class: \"%s\", Error: \"%s\"\n", classname, error);
					dlclose(cd_handler[j].handler);
					cd_handler[j].lid = 0;
					continue;
				}
			}
			sprintf(tmpStr, "SELECT `id`, `key` FROM `%s` WHERE `id`=%u", classname, cdid);
			res1 = stmt->executeQuery(tmpStr);
			// http://www.linuxjournal.com/article/3687?page=0,0
			cd_root->root[cd_root->numOfCloudDrives] = (*cd_handler[j].newCDDriver)(res->getString("key")->c_str(), res->getUInt("id"));
			delete res1;
		}
		cd_root->root[cd_root->numOfCloudDrives]=NULL;
		delete res;
		delete stmt;
		delete get_cloud_drive_list;
		delete get_number_of_library;
	}
	MemManager::free(classname);
	MemManager::free(tmpStr);
}
void Client::loadLogicalDrive(){
	struct client_logical_drive* ld_last;
	struct client_clouddrive* cd_last;
	sql::PreparedStatement* get_logical_drive = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldriveinfo` WHERE `uid`=?");
	sql::PreparedStatement* get_cloud_drive_by_ldid = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldrivecontainer` WHERE `ldid`=?");
	sql::ResultSet *res;
	sql::ResultSet *res1;

	get_logical_drive->setInt(1, account_id);
	res = get_logical_drive->executeQuery();

	ld_root->root = NULL;
	ld_root->numOfLogicalDrive = res->rowsCount();
	while(res->next()){
		if(ld_root){
			ld_last = ( ld_last->next = (struct client_logical_drive*) MemManager::allocate(sizeof(struct client_logical_drive)) );
		}else{
			ld_last = ( ld_root->root = (struct client_logical_drive*) MemManager::allocate(sizeof(struct client_logical_drive)) );
		}

		ld_last->ldid = res->getUInt("ldid");
		ld_last->algoid = res->getUInt("algoid");
		ld_last->name = (char*)MemManager::allocate(strlen(res->getString("name")->c_str())+1);
		strcpy(ld_last->name, res->getString("name")->c_str());
		ld_last->size = res->getUInt64("size");
		ld_last->root = NULL;
		ld_last->next = NULL;

		get_cloud_drive_by_ldid->setUInt(1, ld_last->ldid);
		res1 = get_cloud_drive_by_ldid->executeQuery();

		ld_last->numOfCloudDrives = res1->rowsCount();
		while(res1->next()){
			if(ld_last->root){
				cd_last = ( cd_last->next = (struct client_clouddrive*) MemManager::allocate(sizeof(struct client_clouddrive)) );
			}else{
				cd_last = ( ld_last->root = (struct client_clouddrive*) MemManager::allocate(sizeof(struct client_clouddrive)) );
			}
			cd_last->cdid = res1->getUInt("cdid");
			cd_last->size = res1->getUInt64("size");
			cd_last->dir = (char*)MemManager::allocate(strlen(res1->getString("cddir")->c_str())+1);
			strcpy(cd_last->dir, res1->getString("cddir")->c_str());
			cd_last->next = NULL;
		}
		delete res1;

	}
	delete res;
	delete get_logical_drive;
	delete get_cloud_drive_by_ldid;
}


void Client::prepareStatement(){

	//	used in permission checking
	check_user_logical_drive = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldriveinfo` WHERE `ldid`=? AND `uid`=?");

	//	used in 0x04
	get_list = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `parentid`=?");

	//	used in 0x20
	get_next_fileid = MySQL::getCon()->prepareStatement("SELECT IFNULL((SELECT MAX(fileid)+1 FROM `directory` WHERE `ldid`=1 GROUP BY ldid), 1) as `fileid`");
	create_file = MySQL::getCon()->prepareStatement("INSERT INTO `directory` (`ldid`, `parentid`, `fileid`, `name`, `size`) VALUE (?,?,?,?,?)");

	//	used in 0x21
	insert_chunk = MySQL::getCon()->prepareStatement("INSERT INTO `filechunk` (`ldid`, `cdid`, `fileid`, `seqnum`, `chunk_name`, `size`) VALUE (?,?,?,?,?,?)");

	//	used in 0x22
	get_file_chunk = MySQL::getCon()->prepareStatement("SELECT * FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");

	//	used in 0x28
	get_child = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `parentid`=?");
	get_file = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `fileid`=?");
	del_file = MySQL::getCon()->prepareStatement("DELETE FROM `directory` WHERE `ldid`=? AND `fileid`=?");
}

void Client::updatePrepareStatementAccount(){
	check_user_logical_drive->setInt(2, account_id);
}

void Client::doHandshake(){
//	if not using websocket, how to confirm message exact size in our protocol?
	SecureSocket::recv(inBuffer, WebSocket::MAX_PACKAGE_SIZE, 0);
	SecureSocket::send(outBuffer, WebSocket::getHandShakeResponse(inBuffer, outBuffer, &err));
}

void Client::commandInterpreter(){

	bool isEnd = false;

	readLen = recvLen = 0;

	do{
		readLen = WebSocket::getMsg(inBuffer, SecureSocket::recv(inBuffer, WebSocket::MAX_PACKAGE_SIZE), false, &recvLen, maskKey, &err); // the first message should not be continue
		// err handling
		if(err & WebSocket::ERR_VER_MISMATCH) printf("WebSocket Version not match.\n");
		if(err & WebSocket::ERR_NOT_WEBSOCKET) printf("Connection is not WebScoket.\n");
		if(err & WebSocket::ERR_WRONG_WS_PROTOCOL) printf("WebSocket Protocol Error, Client Package have no mask.\n");
		if(err) break;

		// recv part
		// should check login then do other step
		if(account_id > 0 || !*inBuffer){
			switch(*inBuffer){
				case 0x00: // ls acc req
					Client::readLogin();
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
				case 0x20:
					Client::readCreateFile();
					break;
				case 0x21:
					Client::readSaveFile();
					break;
				case 0x22:
					Client::readGetFile();
					break;
				case 0x28:
					Client::readDelFile();
					break;
				case 0x88:
					Client::addResponseQueue(0x88, NULL);
					isEnd = true;
					break;
			}
		}

	}while(!isEnd);
	pthread_mutex_lock(&client_end_mutex);
	pthread_mutex_lock(&client_end_mutex); // waiting responseThread end
}

/* resposne function */
void Client::addResponseQueue(unsigned char command, void* info){
	struct client_response* tmp = (struct client_response*) MemManager::allocate(sizeof(struct client_response));
	tmp->command = command;
	tmp->info = info;
	tmp->next = NULL;
	pthread_mutex_lock(&res_queue_mutex);
	if(res_root){
		res_last = ( res_last->next = tmp );
	}else{
		res_last = ( res_root = tmp );
	}
	pthread_mutex_unlock(&res_queue_mutex);
	pthread_mutex_unlock(&res_mutex);
}

void Client::responseThread(void* arg){
	bool isEnd = false;
	struct client_response* tmp;
	do{
		pthread_mutex_lock(&res_mutex);
		do{
			pthread_mutex_lock(&res_queue_mutex);
			res_root = ( tmp = res_root )->next;
			pthread_mutex_unlock(&res_queue_mutex);
			if(tmp != NULL){
				switch(tmp->command){
					case 0x00:
					case 0x01:
						Client::sendLogin(tmp->command, tmp->info);
						break;
					case 0x02:
						Client::sendGetCloudDrive();
						break;
					case 0x03:
						Client::sendGetLogicalDrive();
						break;
					case 0x04:
						Client::sendList(tmp->info);
						break;
					case 0x20:
						Client::sendCreateFile(tmp->info);
						break;
					case 0x21:
						Client::sendSaveFile(tmp->info);
						break;
					case 0x22:
						Client::sendGetFileInfo(tmp->info);
						break;
					case 0x23:
						Client::sendGetFileChunk(tmp->info);
						break;
					case 0x28:
						Client::sendDelFile(tmp->info);
						break;
					case 0x88:
						SecureSocket::send(outBuffer, WebSocket::close(outBuffer));
						isEnd = true;
						break;
				}
			}
			MemManager::free(tmp);
		}while(res_root);
	}while(!isEnd); // how to comfirm no thread running ??
	pthread_mutex_unlock(&client_end_mutex);
}

/* Read Command */

// !important, how about login not ok? pointer is not set properly

// 0x00, 0x01 //done
void Client::readLogin(){
	struct client_list_root* info = (struct client_list_root*) MemManager::allocate(sizeof(struct client_list_root));
	char* token = (char*) inBuffer+6;
	sql::PreparedStatement* pstmt;
	sql::ResultSet *res;

	token[32]=0;
	info->operationID = *(inBuffer+1);
	account_id = Network::toInt(inBuffer+2);

	pstmt = MySQL::getCon()->prepareStatement("SELECT * FROM `token` WHERE `token`=? AND `id`=?");
	pstmt->setString(1, token);
	pstmt->setUInt(2, account_id);
	res = pstmt->executeQuery();
	if(res->rowsCount() == 1){
		delete pstmt;
		pstmt = MySQL::getCon()->prepareStatement("DELETE FROM `token` WHERE `id`=?");
		pstmt->setUInt(1, account_id);
		pstmt->executeUpdate();

		Client::updatePrepareStatementAccount();
		Client::loadCloudDrive();
		Client::loadLogicalDrive();
	}else{
		account_id=0;
	}
	delete pstmt;
	Client::addResponseQueue(!!account_id /* 0x00 || 0x01 */ , info);
	delete res;
}
// 0x02 //done
void Client::readGetService(){
	cd_root->operationID = *(inBuffer+1);
	Client::addResponseQueue(0x02, NULL);
}
// 0x03 //done
void Client::readGetDrive(){
	ld_root->operationID = *(inBuffer+1);
	Client::addResponseQueue(0x03, NULL);
}
// 0x04 //done
void Client::readList(){
	struct client_list_root* info = (struct client_list_root*) MemManager::allocate(sizeof(struct client_list_root));
	struct client_list* tmp;
	unsigned int ldid = Network::toInt(inBuffer+2);
	unsigned long long parentid = Network::toLongLong(inBuffer+6);
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
				tmp = ( tmp->next = (struct client_list*) MemManager::allocate(sizeof(struct client_list)) );
			}else{
				info->root = ( tmp = (struct client_list*) MemManager::allocate(sizeof(struct client_list)) );
			}
			tmp->fileid = res->getUInt64("fileid");
			tmp->fileSize = res->getUInt64("size");
			tmp->next = NULL;

			tmp->name = (char*) MemManager::allocate(strlen(res->getString("name")->c_str())+1);
			strcpy(tmp->name, res->getString("name")->c_str());
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
			info->fileid=res->getUInt64("fileid");

			create_file->setUInt(1, logicalDriveID);
			create_file->setUInt64(2, parentID); // what if parentID not exist in database?
			create_file->setUInt64(3, info->fileid);
			create_file->setString(4, name);
			create_file->setUInt64(5, fileSize);
			if(!create_file->executeUpdate()){
				info->fileid = 0;
			}
			break;
		}
	}
	delete res;
	Client::addResponseQueue(0x20, (void*) info);
}
// 0x21 //done
void Client::readSaveFile(){
	struct client_save_file* info = (struct client_save_file*) MemManager::allocate(sizeof(struct client_save_file));
	unsigned short bufShift = 0;
	unsigned long long maxSaveSize;
	int fd;
	sql::ResultSet *res;

	info->object = this;
	info->fptr = &Client::processGetFileChunk;

	info->isInsertOK = 0;
	info->operationID = *(inBuffer +1);
	info->ldid = Network::toInt(inBuffer + 2);
	info->cdid = Network::toInt(inBuffer + 6);
	info->fileid = Network::toLongLong(inBuffer + 10);
	info->seqnum = Network::toInt(inBuffer + 18);
	info->remoteName = (char*) Network::toChars(inBuffer + 26);
	bufShift = 32 + Network::toShort(inBuffer + 26); // 26 + 2 + name.length + 4
	info->chunkSize = (maxSaveSize = Network::toInt(inBuffer + bufShift - 4));

	info->tmpFile = FileManager::newTemp(info->chunkSize);

	fd = FileManager::open(info->tmpFile, 'w');
	write(fd, inBuffer + bufShift, readLen - bufShift);
	maxSaveSize -= readLen - bufShift;
	// check whether this call will have some security problem
	while(recvLen && maxSaveSize > 0 && maxSaveSize < info->chunkSize){
		readLen = WebSocket::parseMsg(inBuffer, SecureSocket::recv(inBuffer, WebSocket::MAX_PACKAGE_SIZE > recvLen ? recvLen : WebSocket::MAX_PACKAGE_SIZE), true, &recvLen, maskKey, &err);
		write(fd, inBuffer, maxSaveSize > readLen ? readLen : maxSaveSize);
		maxSaveSize -= readLen;
	}
	close(fd);

	while(recvLen > 0){
		recvLen -= SecureSocket::recv(inBuffer, WebSocket::MAX_PACKAGE_SIZE > recvLen ? recvLen : WebSocket::MAX_PACKAGE_SIZE);
	}

	check_user_logical_drive->setInt(1, info->ldid);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		insert_chunk->setUInt(1,info->ldid);
		insert_chunk->setUInt(2,info->cdid);
		insert_chunk->setUInt64(3,info->fileid);
		insert_chunk->setUInt(4,info->seqnum);
		insert_chunk->setString(5,info->remoteName);
		insert_chunk->setUInt(6,info->chunkSize);
		info->isInsertOK = insert_chunk->executeUpdate();
	}
	delete res;
	Thread::create(&Client::_thread_redirector, (void*) info);
}
// 0x22 //done
void Client::readGetFile(){
	struct client_read_file_info* info = (struct client_read_file_info*) MemManager::allocate(sizeof(struct client_read_file_info));;
	struct client_read_file* chunk_info;
	unsigned int logicalDriveID = Network::toInt(inBuffer + 2);
	unsigned long long fileID = Network::toLongLong(inBuffer + 6);
	sql::ResultSet *res;

	check_user_logical_drive->setInt(1, logicalDriveID);
	res = check_user_logical_drive->executeQuery();
	if(res->rowsCount() == 1){
		delete res;

		get_file_chunk->setUInt(1, logicalDriveID); // get_file_chunk is undefine
		get_file_chunk->setUInt64(2, fileID);
		res = get_file_chunk->executeQuery();

		info->operationID = *(inBuffer + 1);
		info->num_of_chunk = res->rowsCount();
		Client::addResponseQueue(0x22, (void*) info);

		while(res->next()){
			chunk_info = (struct client_read_file*) MemManager::allocate(sizeof(struct client_read_file));
			chunk_info->object = this;
			chunk_info->fptr = &Client::processGetFileChunk;
			chunk_info->operationID = *(inBuffer + 1);
			chunk_info->ldid = logicalDriveID;
			chunk_info->cdid = res->getUInt("cdid");
			chunk_info->fileid = fileID;
			chunk_info->seqnum = res->getUInt("seqnum");
			chunk_info->chunkSize = res->getUInt("size");

			chunk_info->chunkName = (char*) MemManager::allocate(strlen(res->getString("chunk_name")->c_str())+1);
			strcpy(chunk_info->chunkName, res->getString("chunk_name")->c_str());

			Thread::create(&Client::_thread_redirector, (void*) chunk_info);
		}
	}
	delete res;
}
// 0x28 //done
void Client::readDelFile(){
	struct client_del_file* info;
	unsigned int ldid = Network::toInt(inBuffer + 2);
	unsigned long long fileid = Network::toLongLong(inBuffer + 6) ;
	sql::ResultSet *res;
	get_file->setUInt( 1, ldid );
	get_file->setUInt64( 2, fileid );
	res = get_file->executeQuery();
	if(res->next()){
		info = (struct client_del_file*) MemManager::allocate(sizeof(struct client_del_file));
		info->object = this;
		info->fptr = &Client::processDelFile;
		info->operationID = *(inBuffer+1);
		info->ldid = ldid;
		info->parentid = res->getUInt("parentid");
		info->fileid = fileid;
		info->name = (char*) MemManager::allocate( strlen( res->getString("name").c_str() ) + 1 );
		strcpy(info->name, res->getString("name")->c_str());

		del_file->setUInt( 1, ldid );
		del_file->setUInt64( 2, fileid );
		del_file->executeUpdate();
		if(res->getUInt64("size") > 0){
			Thread::create(&Client::_thread_redirector, (void*) info);
		}else{
			Client::addResponseQueue(0x28, info);
			delete res;
			get_child->setUInt( 1, ldid );
			get_child->setUInt64( 2, fileid );
			res = get_child->executeQuery();
			while(res->next()){
				Network::toBytes((unsigned long long) res->getUInt64(fileid), inBuffer + 6);
				Client::readDelFile();
			}
		}
	}
	delete res;
}

/* Process Command */
// 0x21
void Client::processSaveFile(void *arg){
	struct client_save_file* info = (struct client_save_file*) arg;

	char* dir;
	CDDriver* cdDriver;

	char* remotePath = (char*) MemManager::allocate(512);
	char* localPath  = (char*) MemManager::allocate(512);

	// get remote store directory
	for(struct client_logical_drive* ld = ld_root->root; ld; ld = ld->next){
		if(ld->ldid == info->ldid){
			for(struct client_clouddrive* cd = ld->root; cd; cd = cd->next){
				if(cd->cdid == info->cdid){
					dir = cd->dir;
					break;
				}
			}
			break;
		}
	}

	// get handle Cloud Drive driver
	for(CDDriver** cd = cd_root->root; *cd; cd++){
		if((*cd)->isID(info->cdid)){
			cdDriver = *cd;
			break;
		}
	}
	sprintf(remotePath, "%s%s", dir, info->remoteName);
	FileManager::getTempPath(info->tmpFile, localPath);
	if(cdDriver->put(remotePath, localPath)){
		// not zero mean fail
	}
	MemManager::free(remotePath);
	MemManager::free(localPath);
	Client::addResponseQueue(0x21, info);
}
// 0x23
void Client::processGetFileChunk(void *arg){
	struct client_read_file* info = (struct client_read_file*) arg;

	char* dir;
	CDDriver* cdDriver;
	char* remotePath = (char*) MemManager::allocate(512);
	char* localPath  = (char*) MemManager::allocate(512);
	// get remote store directory
	for(struct client_logical_drive* ld = ld_root->root; ld; ld = ld->next){
		if(ld->ldid == info->ldid){
			for(struct client_clouddrive* cd = ld->root; cd; cd = cd->next){
				if(cd->cdid == info->cdid){
					dir = cd->dir;
					break;
				}
			}
			break;
		}
	}

	// get handle Cloud Drive driver
	for(CDDriver** cd = cd_root->root; *cd; cd++){
		if((*cd)->isID(info->cdid)){
			cdDriver = *cd;
			break;
		}
	}
	info->tmpFile = FileManager::newTemp(info->chunkSize);
	sprintf(remotePath, "%s%s", dir, info->chunkName);
	FileManager::getTempPath(info->tmpFile, localPath);
	if(cdDriver->get(remotePath, localPath)){
		// not zero mean fail
	}

	MemManager::free(remotePath);
	MemManager::free(localPath);
	Client::addResponseQueue(0x23, info);
}
// 0x28
void Client::processDelFile(void *arg){
	struct client_read_file* info = (struct client_read_file*) arg;
	struct client_logical_drive* ld;

	char* dir;
	CDDriver* cdDriver;
	char* remotePath = (char*) MemManager::allocate(512);

	sql::PreparedStatement* pstmt;	
	sql::ResultSet *res;

	// get logical drive info
	for(ld = ld_root->root; ld && ld->ldid != info->ldid; ld = ld->next);

	pstmt = MySQL::getCon()->prepareStatement("SELECT * FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");
	pstmt->setUInt(1, info->ldid);
	pstmt->setUInt64(2, info->fileid);
	res = pstmt->executeQuery();
	// select all file id chunk
	while(res->next()){
		unsigned int cdid = res->getUInt("cdid");
		const char* chunkName = res->getString("chunk_name").c_str();

		for(struct client_clouddrive* cd = ld->root; cd; cd = cd->next){
			if(cd->cdid == info->cdid){
				dir = cd->dir;
				break;
			}
		}

		// get handle Cloud Drive driver
		for(CDDriver** cd = cd_root->root; *cd; cd++){
			if((*cd)->isID(cdid)){
				cdDriver = *cd;
				break;
			}
		}
		sprintf(remotePath, "%s%s", dir, chunkName);
		if(cdDriver->del(remotePath)){
			// not zero mean fail
		}
	}
	delete res;
	delete pstmt;
	// del all file id
	pstmt = MySQL::getCon()->prepareStatement("DELETE FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");
	pstmt->setUInt(1, info->ldid);
	pstmt->setUInt64(2, info->fileid);
	pstmt->executeUpdate();
	delete pstmt;

	MemManager::free(remotePath);

	Client::addResponseQueue(0x28, info);
}

/* Send Result */
// 0x00, 0x01
void Client::sendLogin(unsigned char command, void* a){
	struct client_list_root* info = (struct client_list_root*) a;
	*outBuffer = command;
	*(outBuffer+1) = info->operationID;
	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 2));
	MemManager::free(info);
}

// 0x02
void Client::sendGetCloudDrive(){
	int bufferShift = 4;
	CDDriver ** cd = cd_root->root;

	*outBuffer = 0x02;
	*(outBuffer+1) = cd_root->operationID;
	Network::toBytes(cd_root->numOfCloudDrives, outBuffer+2);
	for(int i=0; cd[i]; bufferShift+=4, i++){
		if(WebSocket::willExceed(bufferShift,4)){
			SecureSocket::sendoutBuffer, WebSocket::sendMsg(outBuffer, outBuffer, bufferShift));
			bufferShift=2;
			*outBuffer = 0x02;
			*(outBuffer+1) = cd_root->operationID;
		}
		Network::toBytes(cd[i]->getID(), outBuffer + bufferShift);
	}
	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, bufferShift));
}

// 0x03
void Client::sendGetLogicalDrive(){
	struct client_logical_drive* ld = ld_root->root;
	struct client_clouddrive* cd;
	int bufferShift = 4;

	*outBuffer = 0x03;
	*(outBuffer+1) = ld_root->operationID;
	Network::toBytes(ld_root->numOfLogicalDrive, outBuffer+2);
	while(ld){
		// 4+4+8+2+n+2+(4+8)*numOfCloudDrives
		if( WebSocket::willExceed(bufferShift, 20+strlen(ld->name)+12*ld->numOfCloudDrives) ){
			SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, bufferShift));
			bufferShift=2;
			*outBuffer = 0x03;
			*(outBuffer+1) = ld_root->operationID;
		}
		Network::toBytes(ld->ldid  , outBuffer + bufferShift);
		Network::toBytes(ld->algoid, outBuffer + bufferShift + 4);
		Network::toBytes(ld->size  , outBuffer + bufferShift + 8);
		Network::toBytes(ld->name  , outBuffer + bufferShift + 16);
		bufferShift += 19 + strlen(ld->name);
		Network::toBytes(ld->numOfCloudDrives, outBuffer + bufferShift - 2);
		for(cd = ld->root; cd; bufferShift+=12){
			Network::toBytes(cd->cdid, outBuffer + bufferShift);
			Network::toBytes(cd->size, outBuffer + bufferShift + 4);
			cd = cd->next;
			// no need free "cd"
		}
		ld = ld->next;
		// no need free "ld"
	}
	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, bufferShift));
}

// 0x04
void Client::sendList(void* a){
	struct client_list_root* info = (struct client_list_root*) a;
	struct client_list* dir = info->root;
	int bufferShift = 4;
	*outBuffer = 0x04;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->numberOfFile, outBuffer+2);
	for(;dir;bufferShift += 17+strlen(dir->name)){
		// 8+8+2+n
		if(WebSocket::willExceed(bufferShift, 17+strlen(dir->name)) ){
			SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, bufferShift));
			bufferShift=2;
			*outBuffer = 0x04;
			*(outBuffer+1) = info->operationID;
		}
		Network::toBytes(dir->fileid  , outBuffer + bufferShift);
		Network::toBytes(dir->fileSize, outBuffer + bufferShift + 8);
		Network::toBytes(dir->name    , outBuffer + bufferShift + 16);
		a = (void*) dir;
		dir = dir->next;
		MemManager::free(a);
	}
	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, bufferShift));
	MemManager::free(info);
}

// 0x20
void Client::sendCreateFile(void* a){
	struct client_make_file* info = (struct client_make_file*) a;
	*outBuffer = 0x20;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->fileid, outBuffer+2);
	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 10));
	MemManager::free(info);
}
// 0x21
void Client::sendSaveFile(void* a){
	struct client_save_file* info = (struct client_save_file*) a;
	*outBuffer = 0x21;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->seqnum, outBuffer + 2);
	*(outBuffer+6) = info->isInsertOK;
	Network::toBytes(info->chunkSize, outBuffer + 7);

	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 11));

	FileManager::deleteTemp(info->tmpFile);
	MemManager::free(info->remoteName);
	MemManager::free(info);
}
// 0x22
void Client::sendGetFileInfo(void* a){
	struct client_read_file_info* info = (struct client_read_file_info*) a;
	*outBuffer = 0x22;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->num_of_chunk, outBuffer + 2);

	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 6));
	MemManager::free(info);
}
// 0x23
void Client::sendGetFileChunk(void* a){
	struct client_read_file* info = (struct client_read_file*) a;
	unsigned long long maxRead = WebSocket::getRemainBufferSize(14);
	unsigned long long readBytes;
	unsigned long long totalReadBytes=0;
	int fd = FileManager::open(info->tmpFile, 'r');
	*outBuffer = 0x23;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->seqnum, outBuffer + 2);
	Network::toBytes(info->chunkSize, outBuffer + 6);
	if(fd > 0 && maxRead > 0){
		do{
			readBytes = read(fd, outBuffer+14, maxRead);
			if(readBytes >= 0){
				totalReadBytes += readBytes;
				Network::toBytes(readBytes, outBuffer + 10);
				SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, readBytes+14));
			}else{
				SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 14));
			}
			Network::toBytes(totalReadBytes, outBuffer + 6);
		}while(maxRead == readBytes);
		close(fd);
	}else{
		SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 14));
	}
	FileManager::deleteTemp(info->tmpFile);
	MemManager::free(info->chunkName);
	MemManager::free(info);
}
// 0x28
void Client::sendDelFile(void* a){
	struct client_del_file* info = (struct client_del_file*) a;
	*outBuffer = 0x28;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->ldid, outBuffer + 2);
	Network::toBytes(info->parentid, outBuffer + 6);
	Network::toBytes(info->fileid, outBuffer + 14);
	Network::toBytes(info->name, outBuffer + 22);

	SecureSocket::send(outBuffer, WebSocket::sendMsg(outBuffer, outBuffer, 23+strlen(info->name)));
	MemManager::free(info->name);
	MemManager::free(info);

}
void* Client::_thread_redirector(void* arg){
	struct client_thread_director* info = (struct client_thread_director*) arg;
	void (Client::*fptr)(void*) = info->fptr;
	(info->object->*fptr)(arg);
}
