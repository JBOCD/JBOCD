void Base64::encode(const unsigned char* in, char* out){ //specific to 20 bytes input
	int i=0;
	int j=0;
	int outLen=0;
	unsigned char arr_1[3];
	unsigned char arr_2[4];

	for(i=0;i<6;i++){
		arr_1[0] = *(in++);
		arr_1[1] = *(in++);
		arr_1[2] = *(in++);

		out[outLen++] = Base64::base64_chars[( arr_1[0]       ) >> 2];
		out[outLen++] = Base64::base64_chars[((arr_1[0] & 0x03) << 4) + (arr_1[1] >> 4)];
		out[outLen++] = Base64::base64_chars[((arr_1[1] & 0x0f) << 2) + (arr_1[2] >> 6)];
		out[outLen++] = Base64::base64_chars[  arr_1[2] & 0x3f];
	}

	arr_1[0] = *(in++);
	arr_1[1] = *in;

	out[outLen++] = Base64::base64_chars[( arr_1[0]       ) >> 2];
	out[outLen++] = Base64::base64_chars[((arr_1[0] & 0x03) << 4) + (arr_1[1] >> 4)];
	out[outLen++] = Base64::base64_chars[((arr_1[1] & 0x0f) << 2)];
	out[outLen++] = '=';
}
