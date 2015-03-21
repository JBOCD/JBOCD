void WebSocket::init(){
	MAX_PACKAGE_SIZE = json_object_get_int(Config::get("socket.maxPackageSize"));
	if(MAX_PACKAGE_SIZE >= 1024){ // no less than 1KB
		MAX_BUFFER_SIZE = MAX_PACKAGE_SIZE + 8;
	}else{
		MAX_PACKAGE_SIZE = 1024;
		MAX_BUFFER_SIZE = 1032;
	}
}

int WebSocket::getHandShakeResponse(unsigned char* request, unsigned char* buf, int* err){
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
int WebSocket::parseMsg(unsigned char* buf, int readLen, bool isContinue,  long long* payloadLen, unsigned char* maskKey, int* err){
	// isContinue ? MSG_DONTWAIT : 0
	// it should be wait for slow network
	*err = ERR_NO_ERR;
	if(readLen && isContinue){
		// continue read
		// nothing done
		// bye
		decode((unsigned long long*) (buf), (unsigned long long*) (buf), maskKey, readLen);
		*payloadLen -= readLen;
	}else if(!readLen || buf[0] & 0x08){
		// close connection opcode
		buf[0] = 0x88;
		buf[1] = 0;
		readLen=1;
		*payloadLen=0;
//	}else if( ! (buf[1] & 0x80) ){
//		// close connection with error
//		// mask is not 1
//		buf[0] = 0x88;
//		buf[1] = 0;
//		readLen = 1;
//		*payloadLen = 1;
//		err && (*err |= ERR_WRONG_WS_PROTOCOL);
	}else{
		*payloadLen = (long long) (buf[1] & 0x7F);
		if(*payloadLen < 126){
			maskKey[0] = buf[2];
			maskKey[1] = buf[3];
			maskKey[2] = buf[4];
			maskKey[3] = buf[5];
			decode((unsigned long long*) (buf+6), (unsigned long long*) (buf), maskKey, readLen-=6);
		}else if(*payloadLen == 126){
			*payloadLen=Network::toShort(buf+2);
			maskKey[0] = buf[4];
			maskKey[1] = buf[5];
			maskKey[2] = buf[6];
			maskKey[3] = buf[7];
			decode((unsigned long long*) (buf+8), (unsigned long long*) (buf), maskKey, readLen-=8);
		}else{
			*payloadLen=Network::toLongLong(buf+2);
			maskKey[0] = buf[10];
			maskKey[1] = buf[11];
			maskKey[2] = buf[12];
			maskKey[3] = buf[13];
			decode((unsigned long long*) (buf+14), (unsigned long long*) (buf), maskKey, readLen-=14);
		}
		*payloadLen -= readLen;
	}
	return readLen;
}
int WebSocket::sendMsg(unsigned char* buf, unsigned char* msg, long long len){
	int insertLen;

	if(len<126){
		memmove(buf+(insertLen=2), msg, len); // 1 byte fin+opcode && 1 byte payload
		buf[1]=(unsigned char) len;
	}else if(len<65536){
		memmove(buf+(insertLen=4), msg, len); // 1 byte fin+opcode && 1+2 bytes payload
		buf[1]=(unsigned char) 0x7E;
		Network::toBytes((short) len, buf+2);
	}else{
		memmove(buf+(insertLen=10), msg, len); // 1 byte fin+opcode && 1+8 bytes payload
		buf[1]=(unsigned char) 0x7F;
		Network::toBytes(len, buf+2);
	}
	buf[0]=0x82;
	return len+insertLen;
}
int WebSocket::close(unsigned char* buf){
	buf[0]=0x88; // fin=1; opcode=8
	buf[1]=0;
	return 2;
}

void WebSocket::decode(unsigned long long* in, unsigned long long* out, unsigned char* maskKey, int len){
	unsigned long long maskKeyLL;
	memcpy(                   &maskKeyLL     , maskKey, 4);
	memcpy( ((unsigned char*) &maskKeyLL) + 4, maskKey, 4);
	int i=0, j=(len+7)/8;
	for(;i<j;i++,in++,out++){
		*out = *in ^ maskKeyLL;
	}
	
	if(j=len%4){
		memcpy( maskKey    , ((unsigned char*)&maskKeyLL)+j, 4-j);
		memcpy( maskKey+4-j,                  &maskKeyLL   , j  );
	}
}

bool WebSocket::willExceed(unsigned long long curLen, unsigned long long addLen){
	curLen += addLen;
	if(curLen < 126) curLen += 2;
	else if(curLen < 65536) curLen += 4;
	else curLen += 10;
	return WebSocket::MAX_PACKAGE_SIZE < curLen;
}
unsigned long long WebSocket::getRemainBufferSize(unsigned long long curLen){
	if(curLen < 126) curLen += 2;
	else if(curLen < 65536) curLen += 4;
	else curLen += 10;
	return WebSocket::MAX_PACKAGE_SIZE - curLen;
}
