Server::Server(){
	int sockfd = 0, connfd = 0;
	struct sockaddr_in server;
	
	printf("Initializing Server ...\n");
	// setting signal
	signal(SIGCHLD, &Server::_client_close);
	conn_count = 0;
	max_conn = json_object_get_int(Config::get("server.maxActiveClient"));
	
	printf("Starting Server ...\n");
	port = (short) json_object_get_int(Config::get("server.port"));


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( ( sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0){
		printf("Could not create socket.");
		exit(1);
	}else{
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(port);

		printf("Binding Server to port %u ...\n", port);
		while( bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0){ 
			perror("Bind failed");
			printf("Retry Binding after %ds ... \n", json_object_get_int(Config::get("server.bindRetryInterval")));
			sleep(json_object_get_int(Config::get("server.bindRetryInterval")));
			printf("Binding Server to port %u ...\n", port);
		}

		printf("Start Listening ... \n");
		listen(sockfd, json_object_get_int(Config::get("server.maxActiveClient")));
		client_conf = (struct client_info*) malloc(sizeof(struct client_info));
		client_conf->sockLen = sizeof(client_conf->conn);
		while(client_conf->connfd = accept(sockfd, (struct sockaddr*)&(client_conf->conn), (socklen_t*)&(client_conf->sockLen))){
			if(max_conn > conn_count){
				// accept client
				switch(child_pid = fork()){
					case 0:
						// child
						close(sockfd); // close listen
						signal(SIGUSR1, &Server::_server_close); // Set signal handler when server close
						
						SecureSocket::startConn(client_conf->connfd);

						// connect to DB
						MySQL::init();

						// create tmpfile pool
						FileManager::newProcess();
						new Client();
						FileManager::endProcess();
						SecureSocket::close();
						exit(0);
					default:
						conn_count++;
					case -1:
						close(client_conf->connfd);

				}
			}else{
				// reject client, max client reach
				close(client_conf->connfd);
			}
		}
	}
}

void Server::_client_close(int signum){
	pid_t pid;
	int status;
	while( ( pid = waitpid(-1, &status, WNOHANG) ) > 0 ){
		conn_count--;
	}
}
void Server::_server_close(int signum){
	// need to create a pid array to manaage
}
