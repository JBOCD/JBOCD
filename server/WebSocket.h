#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include "Base64.h"

// reference: WebSocket Protocol code: "http://itzone.hk/article/article.php?aid=201211192044089879"
// reference: Websocket decode algothm: "https://developer.mozilla.org/en-US/docs/WebSockets/Writing_WebSocket_server"
// standard : WebSocket Protocol: "https://tools.ietf.org/html/rfc6455"

class WebSocket{
	private:
		static unsigned int MAX_PACKAGE_SIZE;

		WebSocket();
		static const char * const WS_GUID;
		static int (* recv)(void *, int);
		static int (* send)(const void *, int);

		static bool isFin;
		static unsigned int readLen;
		static unsigned long long maskKey;
		static unsigned long long payloadLen;

		static unsigned long long decode(unsigned long long* in, unsigned long long* out, unsigned long long maskKey, int len);
		static unsigned int  getHandShakeResponse(unsigned char* request, unsigned char* buf, int* err);	// return buffer size, err return error code, request == buf is safe

	public:
		static const int ERR_NO_ERR;
		static const int ERR_VER_MISMATCH;
		static const int ERR_NOT_WEBSOCKET;
		static const int ERR_WRONG_WS_PROTOCOL;

		static void init();
		static unsigned int getPackageSize();
		static unsigned int getBufferSize();
		static void setRecvHandle(int (* fun)(void *, int));
		static void setSendHandle(int (* fun)(const void *, int));
		static bool accept(unsigned char* inBuffer, unsigned char* outBuffer, int *err);
		static bool hasNext();													// return has next package
		static unsigned int recvMsg(unsigned char* buf, int* err);				// return buffer size, err return error code
		static unsigned int sendMsg(unsigned char* buf, unsigned long long len);// return buffer size
		static unsigned int close(unsigned char* buf);							// return buffer size
		static bool willExceed(unsigned long long curLen, unsigned long long addLen);
		static unsigned long long getRemainBufferSize(unsigned long long curLen);
};

const char * const WebSocket::WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

const int WebSocket::ERR_NO_ERR=0;
const int WebSocket::ERR_VER_MISMATCH=1;
const int WebSocket::ERR_NOT_WEBSOCKET=2;
const int WebSocket::ERR_WRONG_WS_PROTOCOL=4;
//const int WebSocket::ERR_?=8; //next error code 2^n

unsigned int WebSocket::MAX_PACKAGE_SIZE = 0;

bool WebSocket::isFin = false;
unsigned int WebSocket::readLen = 0;
unsigned long long WebSocket::maskKey = 0;
unsigned long long WebSocket::payloadLen = 0;
int (* WebSocket::recv)(void *, int) = NULL;
int (* WebSocket::send)(const void *, int) = NULL;

#include "WebSocket.cpp"

#endif
