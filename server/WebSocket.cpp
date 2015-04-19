void WebSocket::init(){
	MAX_PACKAGE_SIZE = json_object_get_int(Config::get("socket.maxPackageSize"));
	if(MAX_PACKAGE_SIZE < 1024){ // no less than 1KB
		MAX_PACKAGE_SIZE = 1024;
	}
	isFin = false;
	payloadLen = 0;
}

unsigned int WebSocket::getPackageSize(){
	return MAX_PACKAGE_SIZE;
}
unsigned int WebSocket::getBufferSize(){
	return MAX_PACKAGE_SIZE + 64; // send header (10 bytes) < recv decode (64 bytes)
}

void WebSocket::setRecvHandle(int (* fun)(void *, int )){
	recv = fun;
}
void WebSocket::setSendHandle(int (* fun)(const void *, int )){
	send = fun;
}
bool WebSocket::accept(unsigned char* inBuffer, unsigned char* outBuffer, int *err){
	if((*recv)(inBuffer, WebSocket::MAX_PACKAGE_SIZE) == 1){
		(*recv)(inBuffer+1, WebSocket::MAX_PACKAGE_SIZE);
	}
	(*send)(outBuffer, WebSocket::getHandShakeResponse(inBuffer, outBuffer, err));
	return *err;
};
unsigned int WebSocket::getHandShakeResponse(unsigned char* request, unsigned char* buf, int* err){
	int len = strlen((char*)request);
	char* buf_1 = new char[28];
	char* buf_2 = new char[512];
	char* wsa = new char[28];
	int tmp;
	bool isHandle = false;
	err && (*err = ERR_NO_ERR);
	while(sscanf((char*)request, "%27[^:\r]: %511s\r\n",buf_1,buf_2) > 0){
		switch(*buf_1){
			case 'S':
				if(sscanf(buf_1, "Sec-WebSocket-%s", buf_1)){
					switch(*buf_1){
						case 'K':
							strncpy(buf_2+24,WS_GUID, 36);
							SHA1((unsigned char*)buf_2,60, (unsigned char*)buf_1);
							Base64::encode((unsigned char*)buf_1, wsa);
							isHandle = true;
							break;
						case 'V':
							if(sscanf(buf_2,"%d",&tmp)){
								tmp != 13 && err && (*err |= ERR_VER_MISMATCH);
							}
							break;
					}
				}
				break;
		}
		while(len){
			if(*request == '\r' && *(request+1) == '\n'){
				request+=2;
				break;
			}
			request++;
			len--;
		}
	}
	isHandle
		&& sprintf((char*)buf, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.28s\r\nSec-WebSocket-Protocol: JBOCD\r\n\r\n", wsa)
		|| ( err && (*err |= ERR_NOT_WEBSOCKET) );

	delete []buf_1;
	delete []buf_2;
	delete []wsa;

	return 160;
}
bool WebSocket::hasNext(){
	return readLen && (payloadLen || !isFin);
}
unsigned int WebSocket::recvMsg(unsigned char* buf, int* err){
	readLen = (*recv)(buf, !payloadLen || MAX_PACKAGE_SIZE <= payloadLen? MAX_PACKAGE_SIZE : payloadLen); // can be upgrade for efficiency // just error handling // use of isNewMsg
	*err = ERR_NO_ERR;
	if(readLen && payloadLen){
		// continue read
		// nothing done
		// bye.
		maskKey = decode((unsigned long long*) (buf), (unsigned long long*) (buf), maskKey, readLen);
		payloadLen -= readLen;
	}else if(!readLen || buf[0] & 0x08){
		// close connection opcode
		buf[0] = 0x88;
		buf[1] = 0;
		readLen = 1;
		payloadLen = 0;
		isFin = true;
	}else if( ! (buf[1] & 0x80) ){
		// close connection with error
		// mask is not 1
		buf[0] = 0x88;
		buf[1] = 0;
		readLen = 1;
		payloadLen = 0;
		isFin = true;
		err && (*err |= ERR_WRONG_WS_PROTOCOL);
	}else{
		isFin = !!(*buf & 0x80);
		payloadLen = (unsigned long long) (buf[1] & 0x7F);
		if(payloadLen < 126){
			memcpy(                   &maskKey     , buf + 2, 4);
			memcpy( ((unsigned char*) &maskKey) + 4, buf + 2, 4);
			maskKey = decode((unsigned long long*) (buf+6), (unsigned long long*) (buf), maskKey, readLen-=6);
		}else if(payloadLen == 126){
			payloadLen = Network::toShort(buf+2);
			memcpy(                   &maskKey     , buf + 4, 4);
			memcpy( ((unsigned char*) &maskKey) + 4, buf + 4, 4);
			maskKey = decode((unsigned long long*) (buf+8), (unsigned long long*) (buf), maskKey, readLen-=8);
		}else{
			buf[2] &= 0x7F;
			payloadLen=Network::toLongLong(buf+2);
			memcpy(                   &maskKey     , buf + 10, 4);
			memcpy( ((unsigned char*) &maskKey) + 4, buf + 10, 4);
			maskKey = decode((unsigned long long*) (buf+14), (unsigned long long*) (buf), maskKey, readLen-=14);
		}
		payloadLen -= readLen;
	}
	return readLen;
}
unsigned int WebSocket::sendMsg(unsigned char* buf, unsigned long long len){
	int insertLen;

	if(len<126){
		memmove(buf+(insertLen=2), buf, len); // 1 byte fin+opcode && 1 byte payload
		buf[1]=(unsigned char) len;
	}else if(len<65536){
		memmove(buf+(insertLen=4), buf, len); // 1 byte fin+opcode && 1+2 bytes payload
		buf[1]=(unsigned char) 0x7E;
		Network::toBytes((short) len, buf+2);
	}else{
		memmove(buf+(insertLen=10), buf, len); // 1 byte fin+opcode && 1+8 bytes payload
		buf[1]=(unsigned char) 0x7F;
		Network::toBytes(len, buf+2);
	}
	buf[0]=0x82;
	return (*send)(buf, len+insertLen);
}
unsigned int WebSocket::close(unsigned char* buf){
	buf[0]=0x88; // fin=1; opcode=8
	buf[1]=0;
	return (*send)(buf, 2);
}

unsigned long long WebSocket::decode(unsigned long long* in, unsigned long long* out, unsigned long long maskKey, int len){
	unsigned long long tmp = maskKey;
	int i=0, j=(len+63)/64;
	for(;i<j;i++,in+=8,out+=8){
		* out    = * in    ^ maskKey;
		*(out+1) = *(in+1) ^ maskKey;
		*(out+2) = *(in+2) ^ maskKey;
		*(out+3) = *(in+3) ^ maskKey;
		*(out+4) = *(in+4) ^ maskKey;
		*(out+5) = *(in+5) ^ maskKey;
		*(out+6) = *(in+6) ^ maskKey;
		*(out+7) = *(in+7) ^ maskKey;
	}

	if(payloadLen && (j=len%8)){
		memcpy(                   &maskKey         , ((unsigned char*) &tmp) + j, 8-j);
		memcpy( ((unsigned char*) &maskKey) + 8-j,                     &tmp     , j  );
	}
	return maskKey;
}

bool WebSocket::willExceed(unsigned long long curLen, unsigned long long addLen){
	curLen += addLen;
	return WebSocket::MAX_PACKAGE_SIZE < curLen + addLen;
}
unsigned long long WebSocket::getRemainBufferSize(unsigned long long curLen){
	return WebSocket::MAX_PACKAGE_SIZE - curLen;
}
