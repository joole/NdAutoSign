#ifndef NDCODECONVERT_H 
#define NDCODECONVERT_H

#include <string.h>
#include <iconv.h>

class NdCodeConvert
{
private:
	iconv_t cd;
public:
// ����
	NdCodeConvert(const char *from_charset, const char *to_charset) {
		cd = iconv_open(to_charset, from_charset);
	}

	// ����
	~NdCodeConvert() {
		iconv_close(cd);
	}

	// ת�����
	int convert(char *inbuf, int inlen, char *outbuf, int outlen) {
		char **pin = &inbuf;
		char **pout = &outbuf;

		memset(outbuf, 0, outlen);
		iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen);
		return 0;
	}
};


#endif //NDCODECONVERT_H
