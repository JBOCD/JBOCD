Server::Server(Config* conf){
	printf("Starting Server ...\n");
	port = (short) json_object_get_int(conf->get("server.port"));

	int sockfd = 0, connfd = 0;
	struct sockaddr_in server; 
	struct client_info* client;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( ( sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0){
		printf("Could not create socket.");
		exit(1);
	}else{
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(port);

		printf("Binding Server to port %d ...\n", port);
		if( bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0){ 
			perror("Bind failed");
			exit(1);
		}else{
			printf("Start Listening ... \n");
			listen(sockfd, 10); // 10 == max number of waiting accept connection
			client = (struct client_info*) malloc(sizeof(struct client_info));
			client->sockLen = sizeof(client->conn);
			while(client->connfd = accept(sockfd, (struct sockaddr*)&(client->conn), (socklen_t*)&client->sockLen)){
				printf("Incoming Connection detected.\n");
				Thread::clearThread();
				pthread_create(&(client->thread), NULL, &Server::client_thread, (void*) client);
				printf("New Thread %d is created.\n",(int)client->thread);
				client = (struct client_info*) malloc(sizeof(struct client_info));
				client->sockLen = sizeof(client->conn);
			}
		}
	}
}

void* Server::client_thread(void* in){
	struct client_info* conf = (struct client_info*) in;
	long long recvLen=0;
	int readLen=0, err;
	unsigned char maskKey[4];
	unsigned char* buffer = (unsigned char*) malloc(WebSocket::MAX_PACKAGE_SIZE);
	bool isCont = false;
	bool isEnd = false;

	unsigned char packageCode;

	CDDriver ** dropboxList;
	CDDriver ** googleDriveList;

	// database get all list

	// file management
	char* remoteFileNameBuf = new char[1024];
	char* localFileNameBuf = new char[256];

	int* tmpFile = NULL;
	int tmpFilefd = 0;

	// for get, put
	int totalLen, fileRecvTotalLen, packageTotalLen, packageRecvLen, tmpLen;


	// debug variable
	int msgNum = 0;
//	if not using websocket, how to confirm message exact size in our protocol?
	recv(conf->connfd, buffer, WebSocket::MAX_PACKAGE_SIZE, 0);
	write(conf->connfd, buffer, WebSocket::getHandShakeResponse(buffer, buffer, &err));
	do{
		msgNum++;
		isCont = recvLen!=readLen; // if recvLen != readLen, then isCont == true, it mean it continue to read;
		do{
			// forget to update recvLen?
			readLen = WebSocket::getMsg(conf->connfd, buffer, WebSocket::MAX_PACKAGE_SIZE, isCont, &recvLen, maskKey, &err);
			// protocol part
			if(!isCont){
				packageCode = *buffer;
			}
			switch(packageCode){
				case 0x02: // ls acc req
				case 0x04: // ls dir req
					break;
				case 0x20: // put req first recv part
					if(!isCont){
						tmpFile = FileManager::newTemp();
						FileManager::getTempPath(tmpFile, localFileNameBuf);
						if((tmpFilefd = open(localFileNameBuf,O_WRONLY)) != -1){
							totalLen = Network::toInt(buffer+1);
							if(ftruncate(tmpFilefd,totalLen) != -1){
								tmpLen = readLen-9;
								packageTotalLen = Network::toInt(buffer+1);
								fileRecvTotalLen = 0;
								packageRecvLen = packageTotalLen-tmpLen;
								buffer += 9;
							}else{
								close(tmpFilefd);
								tmpFilefd = -1;
							}
						}
					}else if(tmpFilefd != -1){
						tmpLen = readLen;
						packageRecvLen -= readLen;
					}
					if(tmpFilefd != -1){
						write(tmpFilefd,buffer,tmpLen);
					}
					break;
				case 0x21: // put req continue recv part
					// assume that ben is put all file in a one ws package

					break;
				case 0x22: // get req first send part
					*buffer = 0x22;
					tmpFile = FileManager::newTemp();
					FileManager::getTempPath(tmpFile, localFileNameBuf);
						// call api to get file
						//finding file length
					if((tmpFilefd = open(localFileNameBuf,O_RDONLY)) != -1){
						totalLen = lseek(tmpFilefd, 0L, SEEK_END)+1;
						/*
						if((tmpFilefd = open(localFileNameBuf,"r")) != -1){
							totalLen = Network::toInt(buffer+1);
							if(ftruncate(tmpFilefd,totalLen) != -1){
								tmpLen = readLen-9;
								packageTotalLen = Network::toInt(buffer+1);
								fileRecvTotalLen = 0;
								packageRecvLen = packageTotalLen-tmpLen;
								buffer += 9;
							}else{
								close(tmpFilefd);
								tmpFilefd = -1;
							}
						}*/
					}else if(tmpFilefd != -1){
						tmpLen = readLen;
						packageRecvLen -= readLen;
					}
					if(tmpFilefd != -1){
						write(tmpFilefd,buffer,tmpLen);
					}
					break;
				case 0x23: // get req continue send part
					break;
				case 0x28: // delete file opearation

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
					send(conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 12), 0);
*/
			}

			// err handling
			if(err & WebSocket::ERR_VER_MISMATCH) printf("WebSocket Version not match.\n");
			if(err & WebSocket::ERR_NOT_WEBSOCKET) printf("Connection is not WebScoket.\n");
			if(err & WebSocket::ERR_WRONG_WS_PROTOCOL) printf("WebSocket Protocol Error, Client Package have no mask.\n");
		}while( (isCont=recvLen!=readLen));

		// package end do something la
		switch(packageCode){
			case 0x00: // 00000000
				// 4 byte int: send account id
				// 32byte string: token_hash sha256
				// reject first byte = 0x00
				// always accept
				*buffer = 0x01;
				*(buffer+1) = 0x00;
				send(conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 2), 0);
				break;
			case 0x02: // ls acc req
			case 0x04: // ls dir req
				break;
			case 0x20: // put req first recv part
				*buffer = 0x20;
				if(tmpFilefd != -1 && !packageRecvLen){
					*(buffer+1) = 0x00;
					Network::toBytes((int) packageTotalLen,buffer+2);
					send(conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 6), 0);
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
					send(conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 2), 0);
					FileManager::deleteTemp(tmpFile);
					tmpFile = 0;
				}
				break;
			case 0x21: // put req continue recv part

				break;
			case 0x22: // get req first send part
				break;
			case 0x23: // get req continue send part
				break;
			case 0x28: // delete file opearation

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
				send(conf->connfd, buffer, WebSocket::sendMsg(buffer, buffer, 12), 0);
		}
	}while(!isEnd);
	free(buffer);
	close(conf->connfd);
	// end thread
	Thread::addDelQueue((struct thread_queue*) malloc(sizeof(struct thread_queue)),conf->thread, conf);
}
