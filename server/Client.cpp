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

	prepareStatement();
	WebSocket::accept(inBuffer, outBuffer, &err);

	pthread_mutex_init(&res_mutex, NULL);
	pthread_mutex_init(&res_queue_mutex, NULL);
	pthread_mutex_init(&client_end_mutex, NULL);
	pthread_cond_init(&client_end_cond, NULL);
	pthread_cond_init(&res_cond, NULL);
	pthread_create(&(responseThread_tid), NULL, &_thread_redirector, info);
	commandInterpreter();
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
	update_file = MySQL::getCon()->prepareStatement("UPDATE `directory` SET `parentid`=?, `size`=?, `name`=? WHERE `ldid`=? AND `fileid`=?");
	check_logicaldrive_size = MySQL::getCon()->prepareStatement("SELECT `size`-`alloc_size`<=? FROM `logicaldriveinfo` WHERE `ldid`=?");
	update_logicaldrive_size = MySQL::getCon()->prepareStatement("UPDATE `logicaldriveinfo` SET `alloc_size`=`alloc_size`+? WHERE `ldid`=?");

	//	used in 0x21
	check_chunk_size = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT `size` FROM `filechunk` WHERE `ldid`=? AND `cdid`=? AND `fileid`=? AND `seqnum`=?), 0) as ``");
	check_clouddrive_size = MySQL::getCon()->prepareStatement("SELECT IFNULL( (SELECT `size` - `alloc_size` <= ? FROM `logicaldrivecontainer` WHERE `ldid`=? AND `cdid`=? ), 0) as ``");
	update_chunk_info = MySQL::getCon()->prepareStatement("REPLACE INTO `filechunk` (`ldid`, `cdid`, `fileid`, `seqnum`, `chunk_name`, `size`) VALUE (?,?,?,?,?,?)");
	update_clouddrive_alloc_size = MySQL::getCon()->prepareStatement("UPDATE `logicaldrivecontainer` SET `alloc_size`=`alloc_size`+? WHERE `ldid`=? AND `cdid`=?");

	//	used in 0x22
	get_file_chunk = MySQL::getCon()->prepareStatement("SELECT COUNT(`a`.`seqnum`) as `num_of_chunk`, `b`.`size` as `size` FROM `filechunk` as `a`, `directory` as `b` WHERE `a`.`ldid`=? AND `a`.`fileid`=? AND `a`.`ldid`= `b`.`ldid` AND `a`.`fileid`=`b`.`fileid` GROUP BY `b`.`fileid`, `b`.`size`");

	//	used in 0x22, 0x28, 0x29
	get_all_chunk = MySQL::getCon()->prepareStatement("SELECT * FROM `filechunk` WHERE `ldid`=? AND `fileid`=? ORDER BY `ldid`, `fileid`, `seqnum`");

	//	used in 0x28
	del_file = MySQL::getCon()->prepareStatement("DELETE FROM `directory` WHERE `ldid`=? AND `fileid`=?");
	get_file = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `fileid`=?");
	get_child = MySQL::getCon()->prepareStatement("SELECT * FROM `directory` WHERE `ldid`=? AND `parentid`=?");
	//	used in 0x28, 0x29
	remove_chunk = MySQL::getCon()->prepareStatement("DELETE FROM `filechunk` WHERE `ldid`=? AND `fileid`=?");

	// logging
	insert_log = MySQL::getCon()->prepareStatement("INSERT INTO `log` (`ldid`, `cdid`, `fileid`, `seqnum`, `action`, `description`, `size`, `filename`) VALUE (?, ?, ?, ?, ?, ?, ?, ?)");
}

void Client::takeLog(unsigned int ldid, unsigned int cdid, unsigned long long fileid, unsigned int seqnum, const char* action, const char* description, unsigned long long size, char* filename){

	insert_log->setUInt(1, ldid);
	insert_log->setUInt(2, cdid);
	insert_log->setUInt64(3, fileid);
	insert_log->setUInt(4, seqnum);
	insert_log->setString(5, action);
	insert_log->setString(6, description);
	insert_log->setUInt64(7, size);
	if(filename){
		insert_log->setString(8, filename);
	}else{
		insert_log->setString(8, "");
	}
	insert_log->executeUpdate();
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
			addResponseQueue(0x88, NULL);
			break;
		}

		// recv part
		// should check login then do other step
		if(account_id > 0 || !*inBuffer || *inBuffer == 0x88){
			switch(*inBuffer){
				case 0x00: // ls acc req
					readLogin();
					break;
				case 0x02:
					readGetService();
					break;
				case 0x03:
					readGetDrive();
					break;
				case 0x04:
					readList();
					break;
				case 0x20:
					readCreateFile();
					break;
				case 0x21:
					readSaveFile();
					break;
				case 0x22:
					readGetFile();
					break;
				case 0x28:
					readDelFile();
					break;
				case 0x29:
					readDelChunk();
					break;
				case 0x88:
					addResponseQueue(0x88, NULL);
					isEnd = true;
					break;
			}
		}

	}while(!isEnd);
	pthread_mutex_lock(&client_end_mutex);
	pthread_cond_wait(&client_end_cond, &client_end_mutex);
	pthread_mutex_unlock(&client_end_mutex); // waiting response Thread end

	do{
		WebSocket::recvMsg(inBuffer, &err); // comfirm closed
	}while(inBuffer[0] != 0x88);

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
	pthread_mutex_lock(&res_mutex);
	pthread_cond_signal(&res_cond);
	pthread_mutex_unlock(&res_mutex);

	pthread_mutex_unlock(&res_queue_mutex);
}

void Client::responseThread(void* arg){
	bool isEnd = false;
	struct client_response* tmp;
	do{
		pthread_mutex_lock(&res_mutex);
		while(!res_root){
			pthread_cond_wait(&res_cond, &res_mutex);
		}
		pthread_mutex_unlock(&res_mutex);
		while(res_root){
			pthread_mutex_lock(&res_queue_mutex);
			res_root = ( tmp = res_root )->next;
			pthread_mutex_unlock(&res_queue_mutex);
			switch(tmp->command){
				case 0x00:
				case 0x01:
					sendLogin(tmp->command, tmp->info);
					break;
				case 0x02:
					sendGetCloudDrive();
					break;
				case 0x03:
					sendGetLogicalDrive();
					break;
				case 0x04:
					sendList(tmp->info);
					break;
				case 0x20:
					sendCreateFile(tmp->info);
					break;
				case 0x21:
					sendSaveFile(tmp->info);
					break;
				case 0x22:
					sendGetFileInfo(tmp->info);
					break;
				case 0x23:
					sendGetFileChunk(tmp->info);
					break;
				case 0x28:
					sendDelFile(tmp->info);
					break;
				case 0x29:
					sendDelChunk(tmp->info);
					break;
				case 0x88:
					WebSocket::close(outBuffer);
					isEnd = true;
					break;
			}
			MemManager::free(tmp);
		}
	}while(!isEnd); // how to comfirm no thread running ??
	pthread_cond_signal(&client_end_cond);
	MemManager::free(arg);
}

/* Read Command */

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
		delete res;
		loadCloudDrive();
		loadLogicalDrive();
	}else{
		delete res;
		account_id=0;
	}
	delete pstmt;
	addResponseQueue(!!account_id /* 0x00 || 0x01 */ , info);
}
// 0x02 //done
void Client::readGetService(){
	cd_root->operationID = *(inBuffer+1);
	addResponseQueue(0x02, NULL);
}
// 0x03 //done
void Client::readGetDrive(){
	ld_root->operationID = *(inBuffer+1);
	addResponseQueue(0x03, NULL);
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

	addResponseQueue(0x04, (void*)info);
}
// 0x20 //done
void Client::readCreateFile(){
	struct client_make_file* info = (struct client_make_file*) MemManager::allocate(sizeof(struct client_make_file));

	info->operationID = *(inBuffer+1);
	info->ldid = Network::toInt(inBuffer+2);
	info->fileid = Network::toLongLong(inBuffer+6);
	info->parentid = Network::toLongLong(inBuffer+14);
	info->size = Network::toLongLong(inBuffer+22);
	info->name = (char*) Network::toChars(inBuffer+30);
	info->status = 0;
	if(checkLogicalDrive(info->ldid)){
		if(info->fileid == 0){ // create File
			check_logicaldrive_size->setUInt64(1, info->size);
			check_logicaldrive_size->setUInt(2, info->ldid);
			(res = check_logicaldrive_size->executeQuery())->next();
			if(res->getUInt(1)){
				info->status = FILE_SIZE_EXCEED_LD_LIMIT;
			}else{
				get_next_fileid->setUInt(1, info->ldid);
				delete res;
				res = get_next_fileid->executeQuery();
				while(res->next()){
					info->fileid=res->getUInt64("fileid");
					update_logicaldrive_size->setInt64(1, info->size);
					update_logicaldrive_size->setUInt(2, info->ldid);
					update_logicaldrive_size->executeUpdate();

					create_file->setUInt(1, info->ldid);
					create_file->setUInt64(2, info->parentid); // what if parentID not exist in database?
					create_file->setUInt64(3, info->fileid);
					create_file->setString(4, info->name);
					create_file->setUInt64(5, info->size);
					if(!create_file->executeUpdate()){
						info->status = NO_CHANGE;
						takeLog(info->ldid, 0, info->fileid = 0, 0, "Create File", "Fail", info->size, info->name);
					}else{
						info->status = INSERT;
						takeLog(info->ldid, 0, info->fileid, 0, "Create File", "Successful", info->size, info->name);
					}
					break;
				}
			}
			delete res;
		}else{ // update File
			get_file->setUInt(1, info->ldid);
			get_file->setUInt64(2, info->fileid);
			(res = get_file->executeQuery())->next();
			long long diff = info->size - res->getUInt64("size"); // not yet test

			check_logicaldrive_size->setInt64(1, diff);
			check_logicaldrive_size->setUInt(2, info->ldid);
			(res = check_logicaldrive_size->executeQuery())->next();
			if(res->getUInt(1)){
				info->status = FILE_SIZE_EXCEED_LD_LIMIT;
			}else{
				update_logicaldrive_size->setInt64(1, diff);
				update_logicaldrive_size->setUInt(2, info->ldid);
				update_logicaldrive_size->executeUpdate();

				update_file->setUInt64(1, info->parentid);
				update_file->setUInt64(2, info->size);
				update_file->setString(3, info->name);
				update_file->setUInt(4, info->ldid);
				update_file->setUInt64(5, info->fileid);
				if(!update_file->executeUpdate()){
					info->status = NO_CHANGE;
					takeLog(info->ldid, 0, info->fileid = 0, 0, "Update File", "Upload Chunk, but information has no changed", info->size, info->name);
				}else{
					info->status = UPDATE;
					takeLog(info->ldid, 0, info->fileid, 0, "Update File", "Successful", info->size, info->name);
				}
			}
			delete res;
		}
	}
	addResponseQueue(0x20, (void*) info);}
// 0x21 //done
void Client::readSaveFile(){
	struct client_save_file* info = (struct client_save_file*) MemManager::allocate(sizeof(struct client_save_file));
	unsigned short bufShift = 0;
	unsigned long long maxSaveSize;
	int fd;
	long long diff;
	info->object = this;
	info->fptr = &Client::processSaveFile;
	info->retry = 0;
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
			update_chunk_info->setUInt(1,info->ldid);
			update_chunk_info->setUInt(2,info->cdid);
			update_chunk_info->setUInt64(3,info->fileid);
			update_chunk_info->setUInt(4,info->seqnum);
			update_chunk_info->setString(5,info->remoteName);
			update_chunk_info->setUInt(6,info->chunkSize);
			update_clouddrive_alloc_size->setUInt(2, info->ldid);
			update_clouddrive_alloc_size->setUInt(3, info->cdid);

			(res = check_chunk_size->executeQuery())->next();
			if(!(diff = info->chunkSize - res->getUInt64(1))){ // chunk size no update != file name no update
				info->status = NO_CHANGE;
			}else{
				check_clouddrive_size->setInt64(1,diff);
				update_clouddrive_alloc_size->setInt64(1, diff);
				delete res;
				(res = check_clouddrive_size->executeQuery())->next();
				if(res->getUInt(1)){
					info->status = CHUNK_SIZE_EXCEED_CD_LIMIT;
					takeLog(info->ldid, info->cdid, info->fileid, info->seqnum, "Put Chunk", "Chunk size exceed Cloud Drive limit", info->chunkSize, NULL);
				}else{
					if(update_clouddrive_alloc_size->executeUpdate()){
						info->status = INSERT;
					}else{
						info->status = CD_NOT_IN_LD;
						takeLog(info->ldid, info->cdid, info->fileid, info->seqnum, "Put Chunk", "Cloud Drive is not in Logical Drive", info->chunkSize, NULL);
					}
				}
			}
			// executeUpdate return { 1 == Insert / No Change, 2 == update}
			if(info->status >= NO_CHANGE){
				info->status = update_chunk_info->executeUpdate();
				takeLog(info->ldid, info->cdid, info->fileid, info->seqnum, "Put Chunk", info->status == NO_CHANGE ? "Update Chunk but size no changed" : (info->status == INSERT ? "Insert Chunk" : "Update Chunk size"), info->chunkSize, NULL);
			}
			delete res;
			if(info->status >= NO_CHANGE){
				Thread::create(&_thread_redirector, (void*) info, 0);
			}else{
				addResponseQueue(0x21, info);
			}
		}else{
			info->status = CHUNK_SIZE_ZERO_EXCEPTION;
			takeLog(info->ldid, info->cdid, info->fileid, info->seqnum, "Put Chunk", "Fail: Chunk size is zero", info->chunkSize, NULL);
			addResponseQueue(0x21, info);
		}
	}else{
		info->status = PERMISSION_DENY;
		takeLog(info->ldid, info->cdid, info->fileid, info->seqnum, "Put Chunk", "Fail: Permission Deny, Not owner of Logical Drive", info->chunkSize, NULL);
		addResponseQueue(0x21, info);
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
			delete res;
			get_all_chunk->setUInt(1, info->ldid);
			get_all_chunk->setUInt64(2, info->fileID);
			res = get_all_chunk->executeQuery();
			while(res->next()){
				chunk_info = (struct client_read_file*) MemManager::allocate(sizeof(struct client_read_file));
				chunk_info->object = this;
				chunk_info->fptr = &Client::processGetFileChunk;
				chunk_info->retry = 0;
				chunk_info->operationID = *(inBuffer + 1);
				chunk_info->ldid = info->ldid;
				chunk_info->cdid = res->getUInt("cdid");
				chunk_info->fileid = info->fileID;
				chunk_info->seqnum = res->getUInt("seqnum");
				chunk_info->chunkSize = res->getUInt("size");
				chunk_info->status = 0;

				chunk_info->chunkName = (char*) MemManager::allocate(strlen(res->getString("chunk_name")->c_str())+1);
				strcpy(chunk_info->chunkName, res->getString("chunk_name")->c_str());
				Thread::create(&_thread_redirector, (void*) chunk_info, 1);
			}
			takeLog(info->ldid, 0, info->fileID, 0, "Get File", "Successful", info->size, NULL);
			addResponseQueue(0x22, (void*) info);
		}else{
			info->num_of_chunk = 0;
			info->size = 0;
			takeLog(info->ldid, 0, info->fileID, 0, "Get File", "Fail: No chunk", info->size, NULL);
			addResponseQueue(0x22, (void*) info);
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
		update_clouddrive_alloc_size->setUInt(2, ldid);

		res = get_file->executeQuery();
		if(res->next()){
			unsigned int cdid;
			struct client_del_chunk* chunk_info;
			struct client_logical_drive* ld;

			info = (struct client_del_file*) MemManager::allocate(sizeof(struct client_del_file));
			info->operationID = *(inBuffer+1);
			info->ldid = ldid;
			info->parentid = res->getUInt("parentid");
			info->fileid = fileid;
			info->size = res->getUInt64("size");
			info->name = (char*) MemManager::allocate( strlen( res->getString("name").c_str() ) + 1 );
			strcpy(info->name, res->getString("name")->c_str());
			info->command = 0x28;
			pthread_mutex_init(&info->mutex, NULL);

			del_file->setUInt( 1, ldid );
			del_file->setUInt64( 2, fileid );
			del_file->executeUpdate();

			update_logicaldrive_size->setInt64(1, -1L * info->size);
			update_logicaldrive_size->setUInt(2, ldid);
			update_logicaldrive_size->executeUpdate();

			delete res;
			get_all_chunk->setUInt(1, info->ldid);
			get_all_chunk->setUInt64(2, info->fileid);
			res = get_all_chunk->executeQuery();

			info->numOfChunk = res->rowsCount();
			info->deletedChunk = 0;

			takeLog(ldid, 0, fileid, 0, "Delete File", "Succesful", 0, info->name);

			// get logical drive info
			for(ld = ld_root->root; ld && ld->ldid != info->ldid; ld = ld->next);

			for(i=0;res->next();i++){
				cdid = res->getUInt("cdid");
				chunk_info = (struct client_del_chunk*) MemManager::allocate(sizeof(struct client_del_chunk));

				chunk_info->object = this;
				chunk_info->fptr = &Client::processDelChunk;
				chunk_info->retry = 0;
				chunk_info->chunkName = (char*) MemManager::allocate( strlen( res->getString("chunk_name").c_str() ) + 1 );
				strcpy(chunk_info->chunkName, res->getString("chunk_name")->c_str());
				chunk_info->dir = NULL;
				chunk_info->file_info = info;

				for(struct client_clouddrive* cd = ld->root; cd; cd = cd->next){
					if(cd->cdid == cdid){
						chunk_info->dir = cd->dir;
						break;
					}
				}

				if(chunk_info->dir){
				// get handle Cloud Drive driver
					for(chunk_info->cd = cd_root->root; *chunk_info->cd && !(*chunk_info->cd)->isID(cdid); chunk_info->cd++);
					if(*chunk_info->cd){
						update_clouddrive_alloc_size->setInt64(1, -1L * res->getUInt("size"));
						update_clouddrive_alloc_size->setUInt(3, cdid);
						update_clouddrive_alloc_size->executeUpdate();

						Thread::create(&_thread_redirector, (void*) chunk_info, 2);
					}
				}
			}
			remove_chunk->setUInt(1, info->ldid);
			remove_chunk->setUInt64(2, info->fileid);
			remove_chunk->executeUpdate();
			if(!info->numOfChunk){
				addResponseQueue(0x28, info);
			}
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
			readDelFile();
		}
	}
}
// 0x29 //done
void Client::readDelChunk(){
	struct client_del_file* info = (struct client_del_file*) MemManager::allocate(sizeof(struct client_del_file));
	int i;

	info->operationID = *(inBuffer+1);
	info->ldid = Network::toInt(inBuffer + 2);
	info->fileid = Network::toLongLong(inBuffer + 6) ;
	info->command = 0x29;
	pthread_mutex_init(&info->mutex, NULL);

	if(checkLogicalDrive(info->ldid)){
		unsigned int cdid;
		struct client_del_chunk* chunk_info;
		struct client_logical_drive* ld;

		get_all_chunk->setUInt(1, info->ldid);
		get_all_chunk->setUInt64(2, info->fileid);
		res = get_all_chunk->executeQuery();
		takeLog(info->ldid, 0, info->fileid, 0, "Delete File Chunk", "Succesful", 0, NULL);

		info->numOfChunk = res->rowsCount();
		info->deletedChunk = 0;

		// get logical drive info
		for(ld = ld_root->root; ld && ld->ldid != info->ldid; ld = ld->next);

		for(i=0;res->next();i++){
			cdid = res->getUInt("cdid");
			//if(cdid){
				chunk_info = (struct client_del_chunk*) MemManager::allocate(sizeof(struct client_del_chunk));

				chunk_info->object = this;
				chunk_info->fptr = &Client::processDelChunk;
				chunk_info->retry = 0;
				chunk_info->chunkName = (char*) MemManager::allocate( strlen( res->getString("chunk_name").c_str() ) + 1 );
				strcpy(chunk_info->chunkName, res->getString("chunk_name")->c_str());

				for(struct client_clouddrive* cd = ld->root; cd; cd = cd->next){
					if(cd->cdid == cdid){
						chunk_info->dir = cd->dir;
						break;
					}
				}

				// get handle Cloud Drive driver
				for(chunk_info->cd = cd_root->root; *chunk_info->cd && !(*chunk_info->cd)->isID(cdid); chunk_info->cd++);
				if(*chunk_info->cd){
					update_clouddrive_alloc_size->setInt64(1, -1L * res->getUInt("cdid"));
					update_clouddrive_alloc_size->setUInt(2, info->ldid);
					update_clouddrive_alloc_size->setUInt(3, cdid);
					update_clouddrive_alloc_size->executeUpdate();

					Thread::create(&_thread_redirector, (void*) chunk_info, 2);
				}
			//}

		}
		delete res;
		remove_chunk->setUInt(1, info->ldid);
		remove_chunk->setUInt64(2, info->fileid);
		remove_chunk->executeUpdate();
		if(!info->numOfChunk){
			addResponseQueue(0x29, info);
		}
	}else{
		MemManager::free(info);
	}
}

/* Process Command */
// 0x21
void Client::processSaveFile(void *arg){
	struct client_save_file* info = (struct client_save_file*) arg;

	char* dir = NULL;
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

	if(dir){
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
			if(info->retry++ > maxGetTry){
				info->status = RETRY_LIMIT_EXCEED;
			}else{
				Thread::create(&_thread_redirector, (void*) info, 0);
				MemManager::free(remotePath);
				MemManager::free(localPath);
				return;
			}
		}
	}else{
		info->status = CD_NOT_IN_LD;
	}
	MemManager::free(remotePath);
	MemManager::free(localPath);
	addResponseQueue(0x21, info);
}
// 0x23
void Client::processGetFileChunk(void *arg){
	struct client_read_file* info = (struct client_read_file*) arg;

	char* dir = NULL;
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
	if(dir){
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
			if(info->retry++ > maxGetTry){
				info->status = RETRY_LIMIT_EXCEED;
			}else{
				FileManager::deleteTemp(info->tmpFile);
				Thread::create(&_thread_redirector, (void*) info, 1);
				MemManager::free(remotePath);
				MemManager::free(localPath);
				return;
			}
		}
	}
	MemManager::free(remotePath);
	MemManager::free(localPath);
	addResponseQueue(0x23, info);
}
// 0x28
void Client::processDelChunk(void *arg){
	struct client_del_chunk* info = (struct client_del_chunk*) arg;

	char* remotePath = (char*) MemManager::allocate(512);
	int counter;
	if(info->dir){
		sprintf(remotePath, "%s%s", info->dir, info->chunkName);
		if((*(info->cd))->del(remotePath)){
			if(info->retry++ > maxGetTry){
//				info->status = RETRY_LIMIT_EXCEED;
//				addResponseQueue(info->file_info->command, info->file_info);
			}else{
				Thread::create(&_thread_redirector, (void*) info, 2);
				MemManager::free(remotePath);
				return;
			}
		}
		pthread_mutex_lock(&info->file_info->mutex);
		info->file_info->deletedChunk++;
		if(info->file_info->deletedChunk == info->file_info->numOfChunk){
			addResponseQueue(info->file_info->command, info->file_info);
		}
		pthread_mutex_unlock(&info->file_info->mutex);
}else{
	}
	MemManager::free(remotePath);

//	End thread
//	collect all resourse here
	MemManager::free(info->chunkName);
	MemManager::free(info);
//	addResponseQueue(info->command, info);
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
	*(outBuffer+2) = info->status;
	Network::toBytes(info->fileid, outBuffer+3);
	WebSocket::sendMsg(outBuffer, 11);
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
	*(outBuffer+6) = info->status;
	Network::toBytes(info->chunkSize, outBuffer + 7);
	if(fd > 0 && maxRead > 0){
		while(!info->status && (readBytes = read(fd, outBuffer+15, maxRead))){
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
	MemManager::free(info);

}
// 0x29
void Client::sendDelChunk(void* a){
	struct client_del_file* info = (struct client_del_file*) a;
	int i;
	*outBuffer = 0x29;
	*(outBuffer+1) = info->operationID;

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
