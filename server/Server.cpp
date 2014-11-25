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
			perror("Bind failed.");
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
			switch(*buffer){
				case 0xFF:
					isEnd = true;
					break; 
				default:
					for(int i=0;i<readLen;i++){
						putchar(buffer[i]);
					}
					sendMsg(buffer, "hello world", 12);
			}

			// err handling
			if(err & ERR_VER_MISMATCH) printf("WebSocket Version not match.\n");
			if(err & ERR_NOT_WEBSOCKET) printf("Connection is not WebScoket.\n");
			if(err & ERR_WRONG_WS_PROTOCOL) printf("WebSocket Protocol Error, Client Package have no mask.\n");
		}while( (isCont=recvLen!=readLen));
	}while(!isEnd);
	free(buffer);
	close(conf->connfd);
	// end thread
	Thread::addDelQueue((struct thread_queue*) malloc(sizeof(struct thread_queue)),conf->thread, conf);
}
