void SecureSocket::init(){
	int counter = 0;

	SSL_library_init();
	OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
	SSL_load_error_strings();   /* load all error messages */
	

	ctx = SSL_CTX_new(SSLv23_server_method());
	if ( !ctx ){
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
	}
	if(!json_object_get_boolean(Config::get("socket.secure.useSSLv3"))){
		SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
		counter++;
	}
	if(!json_object_get_boolean(Config::get("socket.secure.useTLSv1"))){
		SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1);
		counter++;
	}
	if(!json_object_get_boolean(Config::get("socket.secure.useTLSv1.1"))){
		SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_1);
		counter++;
	}
	if(!json_object_get_boolean(Config::get("socket.secure.useTLSv1.2"))){
		SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_2);
		counter++;
	}
	if(counter >= 4){
		SSL_CTX_free(ctx);
		ctx = NULL;
	}

	if(ctx){
		// load cert if ssl 
		if(SSL_CTX_use_certificate_file(ctx, json_object_get_string(Config::get("socket.secure.certificate"), SSL_FILETYPE_PEM) <= 0){
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}
		if(SSL_CTX_use_PrivateKey_file(ctx, json_object_get_string(Config::get("socket.secure.privateKey"), SSL_FILETYPE_PEM) <= 0){
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}
		if(!SSL_CTX_check_private_key(ctx)){
			fprintf(stderr, "Private key does not match the public certificate\n");
			exit(EXIT_FAILURE);
		}
	}
}

void SecureSocket::startConn(int client_conn){
	SecureSocket::conn = client_conn;
	if(ctx){
		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, client_conn);
		if(SSL_accept(ssl) == -1){
			ERR_print_errors_fp(stderr);
			SSL_free(ssl);
			close(client_conn);
			exit(EXIT_FAILURE);
		}
	}
}
int SecureSocket::send(const void *buf, int num){
	static int (*fun)(const void *buf, int num) = ssl ? &Secure_send : &NON_Secure_send;
	return (*fun)(buf, num);
}
int SecureSocket::recv(void *buf, int num){
	static int (*fun)(const void *buf, int num) = ssl ? &Secure_recv : &NON_Secure_recv;
	return (*fun)(buf, num);
}
int SecureSocket::Secure_send(const void *buf, int num){
	return SSL_write(ssl, buf, num);
}
int SecureSocket::NON_Secure_send(const void *buf, int num){
	return ::send(conn, buf, num, 0);
}
int SecureSocket::Secure_recv(void *buf, int num){
	return SSL_read(ssl, buf, num);
}
int SecureSocket::NON_Secure_recv(void *buf, int num){
	return ::recv(conn, buf, num);
}

void SecureSocket::close(){
	if(ctx){
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		ssl = ctx = NULL;
	}
	::close(conn);
}