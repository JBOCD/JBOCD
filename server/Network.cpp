unsigned short Network::toShort(unsigned char* in){
	unsigned short out = 0;
	return (out | in[0]) << 8 | in[1];
}
unsigned int Network::toInt(unsigned char* in){
	unsigned int out = 0;
	return ((((((out | in[0]) << 8 ) | in[1]) << 8 ) | in[2]) << 8 ) | in[3];
}
unsigned long long Network::toLongLong(unsigned char* in){
	unsigned long long out = 0;
	return ((((((((((((((out | in[0]) << 8 ) | in[1]) << 8 ) | in[2]) << 8 ) | in[3]) << 8) | in[4]) << 8 ) | in[5]) << 8 ) | in[6]) << 8 ) | in[7];
}
unsigned char* Network::toChars(unsigned char* in){
	unsigned char len = *in;
	unsigned char * out = (unsigned char*) MemManager::allocate(len+1);
	memcpy(out, in+1, len);
	out[len]=0;
	return out;
}
void Network::toBytes(unsigned short value, unsigned char* ptr){
	ptr[1] = (unsigned char) ( (value       ) & 0xFF);
	ptr[0] = (unsigned char) ( (value >>  8 ) & 0xFF);
}
void Network::toBytes(short value, unsigned char* ptr){
	Network::toBytes((unsigned short) value, ptr);
}
void Network::toBytes(unsigned int value, unsigned char* ptr){
	ptr[3] = (unsigned char) ( (value       ) & 0xFF);
	ptr[2] = (unsigned char) ( (value >>  8 ) & 0xFF);
	ptr[1] = (unsigned char) ( (value >> 16 ) & 0xFF);
	ptr[0] = (unsigned char) ( (value >> 24 ) & 0xFF);
}
void Network::toBytes(int value, unsigned char* ptr){
	Network::toBytes((unsigned int) value, ptr);
}
void Network::toBytes(unsigned long long value, unsigned char* ptr){
	ptr[7] = (unsigned char) ( (value       ) & 0xFF);
	ptr[6] = (unsigned char) ( (value >>  8 ) & 0xFF);
	ptr[5] = (unsigned char) ( (value >> 16 ) & 0xFF);
	ptr[4] = (unsigned char) ( (value >> 24 ) & 0xFF);
	ptr[3] = (unsigned char) ( (value >> 32 ) & 0xFF);
	ptr[2] = (unsigned char) ( (value >> 40 ) & 0xFF);
	ptr[1] = (unsigned char) ( (value >> 48 ) & 0xFF);
	ptr[0] = (unsigned char) ( (value >> 56 ) & 0xFF);
}
void Network::toBytes(long long value, unsigned char* ptr){
	Network::toBytes((unsigned long long) value, ptr);
}
void Network::toBytes(char* value, unsigned char* ptr){
	unsigned int t = strlen(value);
	strcpy((char*) ptr+1, value);
	ptr[0] = t > 0xff ? 0xff : (unsigned char) t;
}
int Network::getLength(short value, int len){
	return len + 2;
}
int Network::getLength(int value, int len){
	return len + 4;
}
int Network::getLength(long long value, int len){
	return len + 8;
}
int Network::getLength(char* value, int len){
	return len + 1 + strlen(value);
}
