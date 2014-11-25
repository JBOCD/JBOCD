void Base64::encode(const unsigned char* in, char* out){ //specific to 20 bytes input
	int i=0;
	int j=0;
	int outLen=0;
	unsigned char arr_1[3];
	unsigned char arr_2[4];

	for(i=0;i<6;i++){
		arr_1[0] = in[0];
		arr_1[1] = in[1];
		arr_1[2] = in[2];
		in+=3;
		out[0] = Base64::base64_chars[( arr_1[0]       ) >> 2];
		out[1] = Base64::base64_chars[((arr_1[0] & 0x03) << 4) + (arr_1[1] >> 4)];
		out[2] = Base64::base64_chars[((arr_1[1] & 0x0f) << 2) + (arr_1[2] >> 6)];
		out[3] = Base64::base64_chars[  arr_1[2] & 0x3f];
		out+=4;
	}

	arr_1[0] = in[0];
	arr_1[1] = in[1];

	out[0] = Base64::base64_chars[( arr_1[0]       ) >> 2];
	out[1] = Base64::base64_chars[((arr_1[0] & 0x03) << 4) + (arr_1[1] >> 4)];
	out[2] = Base64::base64_chars[((arr_1[1] & 0x0f) << 2)];
	out[3] = '=';
}
