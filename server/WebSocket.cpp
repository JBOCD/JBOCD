WebSocket::init(){
	WebSocket::MAX_READ_SIZE = json_object_get_int(conf->get("socket.maxPackageSize"));
}

bool WebSocket::checkWebSocket(char* header){
	return (*header == 'G' || *header == 'g') && (*header == 'E' || *header == 'e') && (*header == 'T' || *header == 't');
}
int WebSocket::getWebSocketResponse(unsigned char* request, unsigned char* buf){
	int len = 0;
	char* buf_1 = new char[30];
	char* buf_2 = new char[60];
	

}
int WebSocket::recv(int fd, unsigned char* buf, int* err){

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