Client::Client(){
	struct client_thread_director* info = (struct client_thread_director*) MemManager::allocate(sizeof(struct client_thread_director));
	int i;
	maxPutTry = (i=json_object_get_int(Config::get("file.maxUploadRetry"  ))) > 255 ? 255 : i;
	maxGetTry = (i=json_object_get_int(Config::get("file.maxDownloadRetry"))) > 255 ? 255 : i;
	maxDelTry = (i=json_object_get_int(Config::get("file.maxDeleteRetry"  ))) > 255 ? 255 : i;

	setbuf(stdout, NULL);// debug use
	
	info->object = this;
	info->fptr = &Client::responseThread;
	inBuffer = (unsigned char*) MemManager::allocate(WebSocket::getBufferSize());
	outBuffer = (unsigned char*) MemManager::allocate(WebSocket::getBufferSize());
	res_root = res_last = NULL;
	cd_root = NULL;
	ld_root = NULL;

	account_id = 0;

	Client::prepareStatement();
	WebSocket::accept(inBuffer, outBuffer, &err);

	pthread_mutex_init(&res_mutex, NULL);
	pthread_mutex_init(&res_queue_mutex, NULL);
	pthread_mutex_init(&client_end_mutex, NULL);
	pthread_mutex_lock(&res_mutex);
	pthread_create(&(responseThread_tid), NULL, &Client::_thread_redirector, info);
	Client::commandInterpreter();
}
void Client::loadCloudDrive(){
	if(!cd_root){
		sql::PreparedStatement* get_cloud_drive_list = MySQL::getCon()->prepareStatement("SELECT `a`.`cdid` as `cdid`, `a`.`lid` as `lid`, `b`.`dir` as `classname` FROM `clouddrive` as `a`, `libraries` as `b` WHERE `a`.`uid`=? AND `a`.`lid`=`b`.`id`");
		sql::PreparedStatement* get_number_of_library = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT COUNT(DISTINCT `lid`) FROM `clouddrive` WHERE `uid`=?),0) as `num_of_lib`");
		sql::ResultSet* res1;
		sql::Statement* stmt;

		unsigned int cdid;
		unsigned int lid;
		char* classname = (char*) MemManager::allocate(256);
		char* tmpStr = (char*) MemManager::allocate(512);
		char *error;
		int j;

		stmt = MySQL::getCon()->createStatement();

		get_number_of_library->setUInt(1, account_id);
		res = get_number_of_library->executeQuery();

		if(res->next()){
			cd_handler = (struct clouddriver_handler_list*) MemManager::allocate(sizeof(struct clouddriver_handler_list) * (res->getInt("num_of_lib") + 1));
			memset(cd_handler, 0, sizeof(struct clouddriver_handler_list) * (res->getInt("num_of_lib") + 1));
			delete res;

			get_cloud_drive_list->setUInt(1, account_id);
			res = get_cloud_drive_list->executeQuery();

			cd_root = (struct client_clouddrive_root*) MemManager::allocate(sizeof(struct client_clouddrive_root));
			cd_root->root = (CDDriver **) MemManager::allocate(sizeof(CDDriver*) * (res->rowsCount() + 1));
			for(cd_root->numOfCloudDrives=0; res->next(); cd_root->numOfCloudDrives++){
				strcpy(classname, res->getString("classname")->c_str());
				cdid = res->getUInt("cdid");
				lid = res->getUInt("lid");
				for(j=0;cd_handler[j].lid && cd_handler[j].lid != lid;j++);
				if(!cd_handler[j].lid){
					sprintf(tmpStr, "/usr/local/lib/lib%s.so", classname); // path hardcode now
					cd_handler[j].lid = lid;
					cd_handler[j].handler = dlopen(tmpStr, RTLD_NOW | RTLD_LOCAL);
					if(!cd_handler[j].handler){
						printf("[%05d] Cannot load library. Path: \"%s\", Error: \"%s\"\n", (int) getpid(), tmpStr, dlerror());
						cd_handler[j].lid = 0;
						continue;
					}
					cd_handler[j].newCDDriver = (CDDriver* (*)(const char*, unsigned int))dlsym(cd_handler[j].handler,"createObject");
					if((error=dlerror())!=NULL){
						printf("[%05d] Cannot find function \"CDDriver* createObject(const char*, int)\". Class: \"%s\", Error: \"%s\"\n", (int) getpid(), classname, error);
						dlclose(cd_handler[j].handler);
						cd_handler[j].lid = 0;
						continue;
					}
				}
				sprintf(tmpStr, "SELECT `id`, `key` FROM `%s` WHERE `id`=%u", classname, cdid);
				res1 = stmt->executeQuery(tmpStr);
				// http://www.linuxjournal.com/article/3687?page=0,0

				if(res1->next()){
					cd_root->root[cd_root->numOfCloudDrives] = (*(cd_handler[j].newCDDriver))(res1->getString("key")->c_str(), res1->getUInt("id"));
				}else{
					cd_root->numOfCloudDrives--;
				}
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
}
void Client::loadLogicalDrive(){
	if(!ld_root){
		struct client_logical_drive* ld_last;
		struct client_clouddrive* cd_last;
		sql::PreparedStatement* get_logical_drive = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldriveinfo` WHERE `uid`=?");
		sql::PreparedStatement* get_cloud_drive_by_ldid = MySQL::getCon()->prepareStatement("SELECT * FROM `logicaldrivecontainer` WHERE `ldid`=?");
		sql::ResultSet *res1;

		get_logical_drive->setUInt(1, account_id);
		res = get_logical_drive->executeQuery();

		ld_root = (struct client_logical_drive_root*) MemManager::allocate(sizeof(struct client_logical_drive_root));
		ld_root->numOfLogicalDrive = res->rowsCount();
		ld_root->root = NULL;

		while(res->next()){
			if(ld_root->root){
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
}

bool Client::checkLogicalDrive(unsigned int ldid){
	bool result = false;
	struct client_logical_drive* ld;

	if(ld_root){
		for(ld = ld_root->root; ld && ld->ldid != ldid; ld = ld->next);
		result = !!ld;
	}
	return result;
}

void Client::prepareStatement(){

	//	used in 0x04
	get_list = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `parentid`=?");

	//	used in 0x20
	get_next_fileid = MySQL::getCon()->prepareStatement("SELECT IFNULL((SELECT MAX(fileid)+1 FROM `directory` WHERE `ldid`=? GROUP BY ldid), 1) as `fileid`");
	create_file = MySQL::getCon()->prepareStatement("INSERT INTO `directory` (`ldid`, `parentid`, `fileid`, `name`, `size`) VALUE (?,?,?,?,?)");

	//	used in 0x21
	check_chunk_size = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT `size` FROM `filechunk` WHERE `ldid`=? AND `cdid`=? AND `fileid`=? AND `seqnum`=?), 0) as ``");
	check_clouddrive_size = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT `size` - `alloc_size` < ? FROM `logicaldrivecontainer` WHERE `ldid`=? AND `cdid`=? ), 0) as ``");
	check_logicaldrive_size = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT `a`.`size` - SUM(`b`.`alloc_size`) < ? FROM `logicaldriveinfo` as `a`, `logicaldrivecontainer` as `b` WHERE `a`.`ldid`=? AND `a`.`ldid`=`b`.`ldid` ), 0) as ``");
	update_chunk_info = MySQL::getCon()->prepareStatement("REPLACE INTO `filechunk` (`ldid`, `cdid`, `fileid`, `seqnum`, `chunk_name`, `size`) VALUE (?,?,?,?,?,?)");
	update_clouddrive_alloc_size = MySQL::getCon()->prepareStatement("UPDATE `logicaldrivecontainer` SET `alloc_size`=`alloc_size`+? WHERE `ldid`=? AND `cdid`=?");

	//	used in 0x22
	get_file_chunk = MySQL::getCon()->prepareStatement("SELECT COUNT(`a`.`seqnum`) as `num_of_chunk`, `b`.`size` as `size` FROM `filechunk` as `a`, `directory` as `b` WHERE `a`.`ldid`=? AND `a`.`fileid`=? AND `a`.`ldid`= `b`.`ldid` AND `a`.`fileid`=`b`.`fileid` GROUP BY `b`.`fileid`, `b`.`size`");

	//	used in 0x22, 0x28, 0x29
	get_all_chunk = MySQL::getCon()->prepareStatement("SELECT * FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");

	//	used in 0x28
	del_file = MySQL::getCon()->prepareStatement("DELETE FROM `directory` WHERE `ldid`=? AND `fileid`=?");
	get_file = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `fileid`=?");
	get_child = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `parentid`=?");
	//	used in 0x28, 0x29
	remove_chunk = MySQL::getCon()->prepareStatement("DELETE FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");
}


void Client::commandInterpreter(){

	bool isEnd = false;

	do{
		// clear previous buffer
//		memset(inBuffer, 0, WebSocket::MAX_PACKAGE_SIZE);
		readLen = WebSocket::recvMsg(inBuffer, &err);
		// err handling
		if(err & WebSocket::ERR_VER_MISMATCH) printf("[%05d] WebSocket Version not match.\n", (int) getpid());
		if(err & WebSocket::ERR_NOT_WEBSOCKET) printf("[%05d] Connection is not WebScoket.\n", (int) getpid());
		if(err & WebSocket::ERR_WRONG_WS_PROTOCOL) printf("[%05d] WebSocket Protocol Error, Client Package have no mask.\n", (int) getpid());
		if(err){
			Client::addResponseQueue(0x88, NULL);
			break;
		}

		// recv part
		// should check login then do other step
		if(account_id > 0 || !*inBuffer || *inBuffer == 0x88){
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
				case 0x29:
					Client::readDelChunk();
					break;
				case 0x88:
					Client::addResponseQueue(0x88, NULL);
					isEnd = true;
					break;
			}
		}

	}while(!isEnd);
	pthread_mutex_lock(&client_end_mutex);
	pthread_mutex_lock(&client_end_mutex); // waiting response Thread end
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
		while(res_root){
			pthread_mutex_lock(&res_queue_mutex);
			res_root = ( tmp = res_root )->next;
			pthread_mutex_unlock(&res_queue_mutex);
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
				case 0x29:
					Client::sendDelChunk(tmp->info);
					break;
				case 0x88:
					WebSocket::close(outBuffer);
					isEnd = true;
					break;
			}
			MemManager::free(tmp);
		}
	}while(!isEnd); // how to comfirm no thread running ??
	pthread_mutex_unlock(&client_end_mutex);
	MemManager::free(arg);
}

/* Read Command */

// !important, how about login not ok? pointer is not set properly

// 0x00, 0x01 //done
void Client::readLogin(){
	struct client_list_root* info = (struct client_list_root*) MemManager::allocate(sizeof(struct client_list_root));
	char* token = (char*) inBuffer+6;
	sql::PreparedStatement* pstmt;

	token[128]=0;
	info->operationID = *(inBuffer+1);
	account_id = Network::toInt(inBuffer+2);

	pstmt = MySQL::getCon()->prepareStatement("SELECT * FROM `token` WHERE `token`=? AND `id`=?");
	pstmt->setString(1, token);
	pstmt->setUInt(2, account_id);
	res = pstmt->executeQuery();
	if(res->rowsCount() == 1){
// not remove token
//		delete pstmt;
//		pstmt = MySQL::getCon()->prepareStatement("DELETE FROM `token` WHERE `id`=?");
//		pstmt->setUInt(1, account_id);
//		pstmt->executeUpdate();
		delete res;
		Client::loadCloudDrive();
		Client::loadLogicalDrive();
	}else{
		account_id=0;
		delete res;
	}
	delete pstmt;
	Client::addResponseQueue(!!account_id /* 0x00 || 0x01 */ , info);
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

	info->operationID = *(inBuffer+1);
	info->ldid = Network::toInt(inBuffer+2);
	info->root = NULL;
	info->numberOfFile = 0;
	if(checkLogicalDrive(info->ldid)){

		get_list->setUInt(1, info->ldid);
		get_list->setUInt64(2, Network::toLongLong(inBuffer+6));
		res = get_list->executeQuery();
		info->numberOfFile = res->rowsCount();
		while(res->next()){
			if(info->root){
				tmp = (  tmp->next = (struct client_list*) MemManager::allocate(sizeof(struct client_list)) );
			}else{
				tmp = ( info->root = (struct client_list*) MemManager::allocate(sizeof(struct client_list)) );
			}
			tmp->fileid = res->getUInt64("fileid");
			tmp->fileSize = res->getUInt64("size");
			tmp->next = NULL;

			tmp->name = (char*) MemManager::allocate(strlen(res->getString("name")->c_str())+1);
			strcpy(tmp->name, res->getString("name")->c_str());
		}
		delete res;
	}

	Client::addResponseQueue(0x04, (void*)info);
}
// 0x20 //done
void Client::readCreateFile(){
	struct client_make_file* info = (struct client_make_file*) MemManager::allocate(sizeof(struct client_make_file));

	info->operationID = *(inBuffer+1);
	info->ldid = Network::toInt(inBuffer+2);
	info->name = (char*) Network::toChars(inBuffer+22);

	if(checkLogicalDrive(info->ldid)){
		get_next_fileid->setUInt(1, info->ldid);
		res = get_next_fileid->executeQuery();
		while(res->next()){
			info->fileid=res->getUInt64("fileid");
			create_file->setUInt(1, info->ldid);
			create_file->setUInt64(2, Network::toLongLong(inBuffer+6)); // what if parentID not exist in database?
			create_file->setUInt64(3, info->fileid);
			create_file->setString(4, info->name);
			create_file->setUInt64(5, Network::toLongLong(inBuffer+14));
			if(!create_file->executeUpdate()){
				info->fileid = 0;
			}
			break;
		}
		delete res;
	}
	Client::addResponseQueue(0x20, (void*) info);
}
// 0x21 //done
void Client::readSaveFile(){
	struct client_save_file* info = (struct client_save_file*) MemManager::allocate(sizeof(struct client_save_file));
	unsigned short bufShift = 0;
	unsigned long long maxSaveSize;
	int fd;
	unsigned long long diff;
	info->object = this;
	info->fptr = &Client::processSaveFile;

	info->status = 0;
	info->operationID = *(inBuffer +1);
	info->ldid = Network::toInt(inBuffer + 2);
	info->cdid = Network::toInt(inBuffer + 6);
	info->fileid = Network::toLongLong(inBuffer + 10);
	info->seqnum = Network::toInt(inBuffer + 18);
//	info->remoteName = (char*) Network::toChars(inBuffer + 22);
	info->remoteName = (char*) MemManager::allocate(256);
	sprintf(info->remoteName, "JBOCD-chunk-%llu-%u.dontRemove", info->fileid, info->seqnum);
	bufShift = 27 + *(inBuffer + 22); // 22 + 1 + name.length + 4
	info->chunkSize = (maxSaveSize = Network::toInt(inBuffer + bufShift - 4));
	info->tmpFile = FileManager::newTemp(info->chunkSize);


	fd = FileManager::open(info->tmpFile, 'w');
	write(fd, inBuffer + bufShift, readLen - bufShift);
	maxSaveSize -= readLen - bufShift;
	// check whether this call will have some security problem
	while(WebSocket::hasNext()){
		readLen = WebSocket::recvMsg(inBuffer, &err);
		write(fd, inBuffer, maxSaveSize > readLen ? readLen : maxSaveSize);
		maxSaveSize -= readLen;
	}
	close(fd);

	while(WebSocket::hasNext())	WebSocket::recvMsg(inBuffer, &err);

	if(checkLogicalDrive(info->ldid)){
		if(info->chunkSize){
			check_chunk_size->setUInt(1,info->ldid);
			check_chunk_size->setUInt(2,info->cdid);
			check_chunk_size->setUInt64(3,info->fileid);
			check_chunk_size->setUInt(4,info->seqnum);
			check_clouddrive_size->setUInt(2, info->ldid);
			check_clouddrive_size->setUInt(3, info->cdid);
			check_logicaldrive_size->setUInt(2, info->ldid);
			update_chunk_info->setUInt(1,info->ldid);
			update_chunk_info->setUInt(2,info->cdid);
			update_chunk_info->setUInt64(3,info->fileid);
			update_chunk_info->setUInt(4,info->seqnum);
			update_chunk_info->setString(5,info->remoteName);
			update_chunk_info->setUInt(6,info->chunkSize);
			update_clouddrive_alloc_size->setUInt(2, info->ldid);
			update_clouddrive_alloc_size->setUInt(3, info->cdid);

			(res = check_chunk_size->executeQuery())->next();
			if(diff = res->getUInt64(1) - info->chunkSize){ // chunk size no update != file name no update
				info->status = Client::NO_CHANGE;
			}else{
				check_clouddrive_size->setUInt64(1,diff);
				check_logicaldrive_size->setUInt64(1,diff);
				update_clouddrive_alloc_size->setUInt64(1, diff);
				delete res;
				(res = check_clouddrive_size->executeQuery())->next();
				if(res->getUInt(1)){
					info->status = Client::CHUNK_SIZE_EXCEED_CD_LIMIT;
				}else{
					delete res;
					(res = check_logicaldrive_size->executeQuery())->next();
					if(res->getUInt(1)){
						info->status = Client::CHUNK_SIZE_EXCEED_LD_LIMIT;
					}
					if(update_clouddrive_alloc_size->executeUpdate()){
						info->status = Client::INSERT;
					}else{
						info->status = Client::CD_NOT_IN_LD;
					}
				}
			}
			// executeUpdate return { 1 == Insert / No Change, 2 == update}
			if(info->status >= Client::NO_CHANGE){
				info->status = update_chunk_info->executeUpdate() - info->status;
			}
			delete res;
			if(info->status >= Client::NO_CHANGE){
				Thread::create(&Client::_thread_redirector, (void*) info);
			}else{
				Client::addResponseQueue(0x21, info);
			}
		}else{
			info->status = Client::CHUNK_SIZE_ZERO_EXCEPTION;
			Client::addResponseQueue(0x21, info);
		}
	}else{
		info->status = Client::PERMISSION_DENY;
		Client::addResponseQueue(0x21, info);
	}
}

// 0x22 //done
void Client::readGetFile(){
	struct client_read_file_info* info = (struct client_read_file_info*) MemManager::allocate(sizeof(struct client_read_file_info));;
	struct client_read_file* chunk_info;

	info->operationID = *(inBuffer + 1);
	info->ldid = Network::toInt(inBuffer + 2);
	info->fileID = Network::toLongLong(inBuffer + 6);

	if(checkLogicalDrive(info->ldid)){
		get_file_chunk->setUInt(1, info->ldid);
		get_file_chunk->setUInt64(2, info->fileID);
		if((res = get_file_chunk->executeQuery())->next()){

			info->num_of_chunk = res->getUInt("num_of_chunk");
			info->size = res->getUInt64("size");
			Client::addResponseQueue(0x22, (void*) info);
			delete res;
			get_all_chunk->setUInt(1, info->ldid);
			get_all_chunk->setUInt64(2, info->fileID);
			res = get_all_chunk->executeQuery();
			while(res->next()){
				chunk_info = (struct client_read_file*) MemManager::allocate(sizeof(struct client_read_file));
				chunk_info->object = this;
				chunk_info->fptr = &Client::processGetFileChunk;
				chunk_info->operationID = *(inBuffer + 1);
				chunk_info->ldid = info->ldid;
				chunk_info->cdid = res->getUInt("cdid");
				chunk_info->fileid = info->fileID;
				chunk_info->seqnum = res->getUInt("seqnum");
				chunk_info->chunkSize = res->getUInt("size");

				chunk_info->chunkName = (char*) MemManager::allocate(strlen(res->getString("chunk_name")->c_str())+1);
				strcpy(chunk_info->chunkName, res->getString("chunk_name")->c_str());

				Thread::create(&Client::_thread_redirector, (void*) chunk_info);
			}
		}else{
			info->num_of_chunk = 0;
			info->size = 0;
			Client::addResponseQueue(0x22, (void*) info);
		}
		delete res;
	}
}
// 0x28 //done
void Client::readDelFile(){
	struct client_del_file* info;
	unsigned int ldid = Network::toInt(inBuffer + 2);
	unsigned long long fileid = Network::toLongLong(inBuffer + 6) ;
	unsigned long long *fileList;
	int i;

	if(checkLogicalDrive(ldid)){
		get_file->setUInt( 1, ldid );
		get_file->setUInt64( 2, fileid );
		res = get_file->executeQuery();
		if(res->next()){
			info = (struct client_del_file*) MemManager::allocate(sizeof(struct client_del_file));
			info->command = 0x28;
			info->object = this;
			info->fptr = &Client::processDelChunk;
			info->operationID = *(inBuffer+1);
			info->ldid = ldid;
			info->parentid = res->getUInt("parentid");
			info->fileid = fileid;
			info->name = (char*) MemManager::allocate( strlen( res->getString("name").c_str() ) + 1 );
			strcpy(info->name, res->getString("name")->c_str());

			del_file->setUInt( 1, ldid );
			del_file->setUInt64( 2, fileid );
			del_file->executeUpdate();

			delete res;
			get_all_chunk->setUInt(1, info->ldid);
			get_all_chunk->setUInt64(2, info->fileid);
			res = get_all_chunk->executeQuery();
			info->list = (struct client_del_chunk_array*)MemManager::allocate(sizeof(struct client_del_chunk_array) * (res->rowsCount() + 1) );
			for(i=0;res->next();i++){
				info->list[i].cdid = res->getUInt("cdid");
				info->list[i].chunkName = (char*) MemManager::allocate( strlen( res->getString("chunk_name").c_str() ) + 1 );
				strcpy(info->list[i].chunkName, res->getString("chunk_name")->c_str());
			}
			memset(info->list + i, 0, sizeof(struct client_del_chunk_array));
			remove_chunk->setUInt(1, info->ldid);
			remove_chunk->setUInt64(2, info->fileid);
			remove_chunk->executeUpdate();
			Thread::create(&Client::_thread_redirector, (void*) info);
		}
		delete res;

		get_child->setUInt( 1, ldid );
		get_child->setUInt64( 2, fileid );
		res = get_child->executeQuery();
		fileList = (unsigned long long*) MemManager::allocate(sizeof(unsigned long long) * ( res->rowsCount() + 1 ) );
		for(i=0; res->next(); i++){
			fileList[i] = res->getUInt64("fileid");
		}
		fileList[i]=0;
		delete res;
		for(i=0; fileList[i]; i++){
			Network::toBytes(fileList[i], inBuffer + 6);
			Client::readDelFile();
		}
	}
}
// 0x29 //done
void Client::readDelChunk(){
	struct client_del_file* info = (struct client_del_file*) MemManager::allocate(sizeof(struct client_del_file));
	int i;

	info->command = 0x29;
	info->operationID = *(inBuffer+1);
	info->object = this;
	info->fptr = &Client::processDelChunk;
	info->ldid = Network::toInt(inBuffer + 2);
	info->fileid = Network::toLongLong(inBuffer + 6) ;

	if(checkLogicalDrive(info->ldid)){
		get_all_chunk->setUInt(1, info->ldid);
		get_all_chunk->setUInt64(2, info->fileid);
		res = get_all_chunk->executeQuery();
		info->list = (struct client_del_chunk_array*)MemManager::allocate(sizeof(struct client_del_chunk_array) * (res->rowsCount() + 1) );
		for(i=0;res->next();i++){
			info->list[i].cdid = res->getUInt("cdid");
			info->list[i].chunkName = (char*) MemManager::allocate( strlen( res->getString("chunk_name").c_str() ) + 1 );
			strcpy(info->list[i].chunkName, res->getString("chunk_name")->c_str());
		}
		memset(info->list + i, 0, sizeof(struct client_del_chunk_array));
		delete res;
		remove_chunk->setUInt(1, info->ldid);
		remove_chunk->setUInt64(2, info->fileid);
		remove_chunk->executeUpdate();
		Thread::create(&Client::_thread_redirector, (void*) info);
	}else{
		MemManager::free(info);
	}
}

/* Process Command */
// 0x21
void Client::processSaveFile(void *arg){
	struct client_save_file* info = (struct client_save_file*) arg;

	char* dir;
	CDDriver* cdDriver;

	char* remotePath = (char*) MemManager::allocate(512);
	char* localPath  = (char*) MemManager::allocate(512);

	int counter;

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

	for(counter = 0; counter < maxGetTry && cdDriver->put(remotePath, localPath); counter++);
	if(counter >= maxGetTry) info->status = RETRY_LIMIT_EXCEED;

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
	int counter;

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

	for(counter = 0; counter < maxGetTry && cdDriver->get(remotePath, localPath); counter++);
	if(counter >= maxGetTry) info->status = RETRY_LIMIT_EXCEED;

	MemManager::free(remotePath);
	MemManager::free(localPath);
	Client::addResponseQueue(0x23, info);
}
// 0x28
void Client::processDelChunk(void *arg){
	struct client_del_file* info = (struct client_del_file*) arg;
	struct client_logical_drive* ld;

	char* dir;
	CDDriver* cdDriver;
	char* remotePath = (char*) MemManager::allocate(512);
	int counter;

	// get logical drive info
	for(ld = ld_root->root; ld && ld->ldid != info->ldid; ld = ld->next);

	// select all file id chunk
	for(int i=0; info->list[i].cdid; i++){

		for(struct client_clouddrive* cd = ld->root; cd; cd = cd->next){
			if(cd->cdid == info->list[i].cdid){
				dir = cd->dir;
				break;
			}
		}

		// get handle Cloud Drive driver
		for(CDDriver** cd = cd_root->root; *cd; cd++){
			if((*cd)->isID(info->list[i].cdid)){
				cdDriver = *cd;
				break;
			}
		}
		sprintf(remotePath, "%s%s", dir, info->list[i].chunkName);
		for(counter = 0; counter < maxDelTry && cdDriver->del(remotePath); counter++);
//		info->list[i].status = counter < maxDelTry ? DELETE : RETRY_LIMIT_EXCEED;
	}
	MemManager::free(remotePath);

	Client::addResponseQueue(info->command, info);
}

/* Send Result */
// 0x00, 0x01
void Client::sendLogin(unsigned char command, void* a){
	struct client_list_root* info = (struct client_list_root*) a;
	*outBuffer = command;
	*(outBuffer+1) = info->operationID;
	WebSocket::sendMsg(outBuffer, 2);
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
			WebSocket::sendMsg(outBuffer, bufferShift);
			bufferShift=2;
			*outBuffer = 0x02;
			*(outBuffer+1) = cd_root->operationID;
		}
		Network::toBytes(cd[i]->getID(), outBuffer + bufferShift);
	}
	WebSocket::sendMsg(outBuffer, bufferShift);
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
			WebSocket::sendMsg(outBuffer, bufferShift);
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
	WebSocket::sendMsg(outBuffer, bufferShift);
}

// 0x04
void Client::sendList(void* a){
	struct client_list_root* info = (struct client_list_root*) a;
	struct client_list* dir = info->root;
	int bufferShift = 4;
	*outBuffer = 0x04;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->numberOfFile, outBuffer+2);
	while(dir){
		// 8+8+2+n
		if(WebSocket::willExceed(bufferShift, 17+strlen(dir->name)) ){
			WebSocket::sendMsg(outBuffer, bufferShift);
			bufferShift=2;
			*outBuffer = 0x04;
			*(outBuffer+1) = info->operationID;
		}
		Network::toBytes(dir->fileid  , outBuffer + bufferShift);
		Network::toBytes(dir->fileSize, outBuffer + bufferShift + 8);
		Network::toBytes(dir->name    , outBuffer + bufferShift + 16);
		bufferShift += 17+strlen(dir->name);
		a = (void*) dir;
		MemManager::free(dir->name);
		dir = dir->next;
		MemManager::free(a);
	}
	WebSocket::sendMsg(outBuffer, bufferShift);
	MemManager::free(info);
}

// 0x20
void Client::sendCreateFile(void* a){
	struct client_make_file* info = (struct client_make_file*) a;
	*outBuffer = 0x20;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->fileid, outBuffer+2);
	WebSocket::sendMsg(outBuffer, 10);

	MemManager::free(info->name);
	MemManager::free(info);
}
// 0x21
void Client::sendSaveFile(void* a){
	struct client_save_file* info = (struct client_save_file*) a;
	*outBuffer = 0x21;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->seqnum, outBuffer + 2);
	*(outBuffer+6) = info->status;
	Network::toBytes(info->chunkSize, outBuffer + 7);

	WebSocket::sendMsg(outBuffer, 11);

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
	Network::toBytes(info->size, outBuffer + 6);

	WebSocket::sendMsg(outBuffer, 14);
	MemManager::free(info);
}
// 0x23
void Client::sendGetFileChunk(void* a){
	struct client_read_file* info = (struct client_read_file*) a;
	unsigned long long maxRead = WebSocket::getRemainBufferSize(14);
	unsigned int readBytes;
	unsigned int totalReadBytes=0;
	int fd = FileManager::open(info->tmpFile, 'r');
	*outBuffer = 0x23;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->seqnum, outBuffer + 2);
	Network::toBytes(info->status, outBuffer + 6);
	Network::toBytes(info->chunkSize, outBuffer + 7);
	if(fd > 0 && maxRead > 0){
		while(readBytes = read(fd, outBuffer+15, maxRead)){
			if(readBytes > 0){
				totalReadBytes += readBytes;
				Network::toBytes(readBytes, outBuffer + 11);
				WebSocket::sendMsg(outBuffer, readBytes+15);
			}
			Network::toBytes(totalReadBytes, outBuffer + 7);
		}
		Network::toBytes(readBytes, outBuffer + 11);
		WebSocket::sendMsg(outBuffer, 15);
		close(fd);
	}else{
		Network::toBytes((unsigned int) 0, outBuffer + 11);
		WebSocket::sendMsg(outBuffer, 15);
	}
	FileManager::deleteTemp(info->tmpFile);
	MemManager::free(info->chunkName);
	MemManager::free(info);
}
// 0x28
void Client::sendDelFile(void* a){
	struct client_del_file* info = (struct client_del_file*) a;
	int i;
	*outBuffer = 0x28;
	*(outBuffer+1) = info->operationID;
	Network::toBytes(info->ldid, outBuffer + 2);
	Network::toBytes(info->parentid, outBuffer + 6);
	Network::toBytes(info->fileid, outBuffer + 14);
	Network::toBytes(info->name, outBuffer + 22);

	WebSocket::sendMsg(outBuffer, 23+strlen(info->name));
	MemManager::free(info->name);

	for(i=0;info->list[i].cdid;i++){
		MemManager::free(info->list[i].chunkName);
	}
	MemManager::free(info->list);
	MemManager::free(info);

}
// 0x29
void Client::sendDelChunk(void* a){
	struct client_del_file* info = (struct client_del_file*) a;
	int i;
	*outBuffer = 0x28;
	*(outBuffer+1) = info->operationID;

	for(i=0;info->list[i].cdid;i++){
		MemManager::free(info->list[i].chunkName);
	}
	MemManager::free(info->list);
	MemManager::free(info);

}
void* Client::_thread_redirector(void* arg){
	struct client_thread_director* info = (struct client_thread_director*) arg;
	void (Client::*fptr)(void*) = info->fptr;
	(info->object->*fptr)(arg);
}

Client::~Client(){
	int i;
	struct client_logical_drive* ld;
	struct client_clouddrive* cd;
	struct client_response* res;

	pthread_join(responseThread_tid, NULL);

	delete get_cloud_drive_list;
	delete get_number_of_library;
	delete get_list;
	delete get_next_fileid;
	delete create_file;
	delete check_chunk_size;
	delete check_clouddrive_size;
	delete check_logicaldrive_size;
	delete update_chunk_info;
	delete update_clouddrive_alloc_size;
	delete get_file_chunk;
	delete get_child;
	delete get_file;
	delete del_file;

	MemManager::free(inBuffer);
	MemManager::free(outBuffer);

	for(ld=ld_root->root; ld; ld=ld_root->root){
		ld_root->root = ld->next;
		for(cd=ld->root; cd; cd=ld->root){
			MemManager::free(cd->dir);
			MemManager::free(cd);
		}
		MemManager::free(ld->name);
		MemManager::free(ld);
	}
	MemManager::free(ld_root);

	for(i=0;cd_root->root[i];i++){
		delete cd_root->root[i];
	}
	MemManager::free(cd_root->root);
	MemManager::free(cd_root);

	for(i=0;cd_handler[i].handler;i++){
		dlclose(cd_handler[i].handler);
	}
	MemManager::free(cd_handler);

	for(res=res_root;res;res=res_root){
		res_root=res->next;
		MemManager::free(res);
	}
}
