#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <cstring>
#include "Base64.h"

// reference: WebSocket Protocol code: "http://itzone.hk/article/article.php?aid=201211192044089879"

class WebSocket{
	private:
		WebSocket();
		static const char const* WS_GUID;
		static const int ERR_BUF_OVERFLOW;
		static int MAX_READ_SIZE;
	public:
		static void init();
		static bool checkWebSocket(char* header);
		static int  getWebSocketResponse(unsigned char* request, unsigned char* buf);	// return buffer size
		static int  recv(int fd, unsigned char* buf, int* err);							// return buffer size
		static int  send(unsigned char* msg, int len, unsigned char* buf);				// return buffer size
		static int  close(unsigned char* buf);											// return buffer size
};

static const char const* WebSocket::WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static const int WebSocket::ERR_BUF_OVERFLOW=1;

#include "WebSocket.cpp"

#endif
