#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <cstring>
#include <openssl/sha.h>
#include "Base64.h"

// reference: WebSocket Protocol code: "http://itzone.hk/article/article.php?aid=201211192044089879"

class WebSocket{
	private:
		WebSocket();
		static const char * const WS_GUID;

		static const int ERR_NO_ERR;
		static const int ERR_VER_MISMATCH;
		static const int ERR_NOT_WEBSOCKET;
		static const int ERR_WRONG_WS_PROTOCOL;

		static int MAX_PACKAGE_SIZE;
		static int MAX_CONTENT_SIZE;
	public:
		static void init(Config* conf);
		static int  getHandShakeResponse(unsigned char* request, unsigned char* buf, int* err);				// return buffer size
		static int  recv(int fd, unsigned char* buf, int size, bool isWait, long long* payloadLen, int* err);	// return buffer size, errPtr return error code
		static int  send(unsigned char* buf, unsigned char* msg, long long len);									// return buffer size
		static int  close(unsigned char* buf);																// return buffer size
};

const char * const WebSocket::WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

const int WebSocket::ERR_NO_ERR=0;
const int WebSocket::ERR_VER_MISMATCH=1;
const int WebSocket::ERR_NOT_WEBSOCKET=2;
const int WebSocket::ERR_WRONG_WS_PROTOCOL=4;
//static const int WebSocket::ERR_?=4; //next error code 2^n

#include "WebSocket.cpp"

#endif