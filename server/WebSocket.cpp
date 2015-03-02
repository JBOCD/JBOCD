void WebSocket::init(){
	MAX_PACKAGE_SIZE = json_object_get_int(Config::get("socket.maxPackageSize"));
	if(MAX_PACKAGE_SIZE < 1024){ // no less than 1KB
		MAX_PACKAGE_SIZE = 1024;
	}
}

int WebSocket::getHandShakeResponse(unsigned char* request, unsigned char* buf, int* err){
	int len = 0;
	char* buf_1 = new char[28];
	char* buf_2 = new char[512];
	char* wsa = new char[28];
	int tmp;
	bool isHandle = false;
	err && (*err = ERR_NO_ERR);
	while(sscanf((char*)request, "%[^:\r]: %s\r\n",buf_1,buf_2) > 0){
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
		while(1){
			if(*request == '\r' && *(request+1) == '\n'){
				request+=2;
				break;
			}
			request++;
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
int WebSocket::getMsg(int fd, unsigned char* buf, int size, bool isContinue,  long long* payloadLen, unsigned char* maskKey, int* err){
	// isContinue ? MSG_DONTWAIT : 0
	// it should be wait for slow network
	int readLen = recv(fd, buf, size, 0); 
	*err = ERR_NO_ERR;
	if(readLen && isContinue){
		// continue read
		// nothing done
		// bye
		decode(buf, buf, maskKey, readLen);
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
			decode(buf+6, buf, maskKey, readLen-=6);
		}else if(*payloadLen == 126){
			*payloadLen=Network::toShort(buf+2);
			maskKey[0] = buf[4];
			maskKey[1] = buf[5];
			maskKey[2] = buf[6];
			maskKey[3] = buf[7];
			decode(buf+8, buf, maskKey, readLen-=8);
		}else{
			*payloadLen=Network::toLongLong(buf+2);
			maskKey[0] = buf[10];
			maskKey[1] = buf[11];
			maskKey[2] = buf[12];
			maskKey[3] = buf[13];
			decode(buf+14, buf, maskKey, readLen-=14);
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

void WebSocket::decode(unsigned char* in, unsigned char* out, unsigned char* maskKey, int len){
//	unsigned char tmp[4];
	unsigned long long* inLL = (unsigned long long*) in;
	unsigned long long* outLL = (unsigned long long*) out;
	unsigned long long maskKeyLL;
	memcpy( ((void*) &maskKeyLL)    , maskKey, 4);
	memcpy( ((void*) &maskKeyLL) + 4, maskKey, 4);
	int i=0, j=len/8;
//	int i=0, j=len/4;
	for(;i<j;i++,inLL++,outLL++){
//	for(;i<j;i++){
		*outLL = *inLL ^ maskKeyLL;
/*
		out[0] = in[0] ^ maskKey[0];
		out[1] = in[1] ^ maskKey[1];
		out[2] = in[2] ^ maskKey[2];
		out[3] = in[3] ^ maskKey[3];

		out+=4;	in+=4;
*/
	}
	j=len%8;
	out = (unsigned char*) outLL;
	in = (unsigned char*) inLL;
/*
	if(j > 4){
		*((int*) out) = *((int*)in) ^ (int)maskKeyLL;
		out+=4;	in+=4;
		j-=4;
	}
	for(i=0;i<4;i++){
		if(i<j){
			tmp[i+(4-j)] = maskKey[i];
			out[i] = in[i] ^ maskKey[i];
		}else{
			tmp[i-j] = maskKey[i];
		}
	}
	memcpy( maskKey, tmp, 4);
*/
	switch(j){
		case 7:
			out[6] = in[6] ^ maskKey[2];
		case 6:
			out[5] = in[5] ^ maskKey[1];
		case 5:
			out[4] = in[4] ^ maskKey[0];
		case 4:
			out[3] = in[3] ^ maskKey[3];
		case 3:
			out[2] = in[2] ^ maskKey[2];
		case 2:
			out[1] = in[1] ^ maskKey[1];
		case 1:
			j=len%4;
			out[0] = in[0] ^ maskKey[0];
			memcpy( maskKey, ((void*)&maskKeyLL)+j, 4-j);
			memcpy( maskKey+4-j, (void*)&maskKeyLL, j);
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
