#ifndef BASE64_H
#define BASE64_H

// reference: Base64: "http://www.adp-gmbh.ch/cpp/common/base64.html"

class Base64{
	private:
		Base64();
		static char * base64_chars;
	public:
		static void encode(const unsigned char* in, char* out); // input len should always 20
//		static char* decode(unsigned const char* in, char* out); // not implement
};

char * Base64::base64_chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#include "Base64.cpp"

#endif
