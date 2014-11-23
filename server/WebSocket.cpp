WebSocket::init(){
	WebSocket::MAX_PACKAGE_SIZE = json_object_get_int(conf->get("socket.maxPackageSize"));
	WebSocket::MAX_CONTENT_SIZE = json_object_get_int(conf->get("socket.maxContentSize"));
}

int WebSocket::getHandShakeResponse(unsigned char* request, unsigned char* buf, int* err){
	int len = 0;
	char* buf_1 = new char[61];
	char* buf_2 = new char[61];
	int tmp;
	bool isHandle = false;
	err && (*err = WebSocket::ERR_NO_ERR);

	while(sscanf(str, "%[^:\r]: %s\r\n",buf_1,buf_2)){
		switch(*buf_1){
			case 'S':
				if(sscanf(buf_1, "Sec-WebSocket-%s", buf_1)){
					switch(buf_1){
						case 'K':
							strncpy(buf_2+24,WebSocket::WS_GUID, 36);
							SHA1((unsigned char*)buf_2,60, (unsigned char*)buf_1);
							Base64::encode((unsigned char*)buf_1, buf_2);
							sprintf(buf, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.28s\r\nSec-WebSocket-Protocol: JBOCD\r\n\r\n", buf_2);
							isHandle = true;
							break;
						case 'V':
							if(sscanf(buf_2,"%d",tmp)){
								(tmp != 13) && err && (*err |= ERR_VER_MISMATCH);
							}
							break;
					}
				}
				break;
		}
		while(1){
			if(*str == '\r' && *(str+1) == '\n'){
				str+=2;
				break;
			}
			str++;
		}
	}
	isHandle || err && (*err |= ERR_NOT_WEBSOCKET);
	delete buf_1;
	delete buf_2;
}
int WebSocket::recv(int fd, unsigned char* buf, int size, bool isWait, long* payloadLen, int* err){
	int readLen = recv(fd, buf, size, isWait ? 0 : MSG_DONTWAIT);
	if(!isWait){
		// continue read
		// nothing done
		// bye
	}else if((*buf) & 0x08){
		// close connection opcode
	}else if( ! (*(buf+1) & 0x80) ){
		// close connection with error
		// mask is not 1
		err & (*err |= ERR_WRONG_WS_PROTOCOL);
	}else{
		// check mask code == 1
		*payloadLen = (long) (*(buf+1) & 0x7F);
		if(*payloadLen < 126){
// in == buf + 6
// out == buf
		}else if(*payloadLen == 126){
// in == buf + 8
// out == buf
		}else{
// in == buf + 14
// out == buf
		}
	}
	return readLen;
}
int WebSocket::send(unsigned char* buf, unsigned char* msg, long len){
	int headerLen;
	*buf=0x82;
	if(len<126){
		totalLen = 2; // 1 byte fin+opcode && 1 byte payload
		*(buf+1)=(unsigned char) len;
		buf = buf+2;
	}else if(len<65536){
		totalLen = 4; // 1 byte fin+opcode && 1+2 bytes payload
		*(buf+1)=0x7E;
		Network::toBytes((short) len, *(buf+2));
		buf = buf+4;
	}else{
		totalLen = 10; // 1 byte fin+opcode && 1+8 bytes payload
		*(buf+1)=0x7F;
		Network::toBytes(len, *(buf+2));
		buf = buf+10;
	}
	memcpy(buf, msg, len);
	return len+totalLen;
}
int WebSocket::close(unsigned char* buf){
	*buf=0x88; // fin=1; opcode=8
	*(buf+1)=0;
	return 2;
}