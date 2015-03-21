#ifndef SECURE_SOCKET_H
#define SECURE_SOCKET_H

#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"

// reference: SSL code: "http://simplestcodings.blogspot.hk/2010/08/secure-server-client-using-openssl-in-c.html"
// documentation : WebSocket Protocol: "https://www.openssl.org/docs/ssl/SSL_CTX_set_options.html"

class SecureSocket{
	private:
		static SSL_CTX *ctx;
		static SSL *ssl;
		static int conn;
		static void ssl_read();
		static int Secure_send(const void *buf, int num);
		static int NON_Secure_send(const void *buf, int num);
		static int Secure_recv(void *buf, int num);
		static int NON_Secure_recv(void *buf, int num);
		SecureSocket();
	public:
		static void init();
		static void startConn();
		static int recv(void *buf, int num);
		static int send(const void *buf, int num);
		static void SecureSocket::close();
};
SSL_CTX *SecureSocket::ctx = NULL;
SSL *SecureSocket::ssl = NULL;
int SecureSocket::conn = 0;

#include "SecureSocket.cpp"

#endif