Server::Server(Config* conf){
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

		if( bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0){ 
			perror("Bind failed.");
			exit(1);
		}else{
			listen(sockfd, 10); // 10 == max number of waiting accept connection
			client = (struct client_info*) malloc(sizeof(struct client_info));
			while(client->connfd = accept(sockfd, (struct sockaddr*)&(client->conn), sizeof(client->conn))){
				Thread::clearThread();
				// create new thread
				pthread_create(&(client->thread), NULL, &Server::client_thread, client);

				client = (struct client_info*) malloc(sizeof(struct client_info));
//
//				close(connfd);
//				sleep(1);
			}
		}
	}
}

Server::client_thread(void* in){
	struct client_info* conf = (struct client_info*) in;
	char* str = "ggwp";
//				snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
	write(conf->connfd, str, strlen(str));
	// end thread
	close(conf->connfd);
	Thread::addDelQueue((struct thread_queue*) malloc(sizeof(struct thread_queue)),conf->thread, conf);
}
