#include "NdCalculateUtil.h"
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>			// Base64 Encode & Decode

#include <string.h>

#include <time.h>
#include <sys/time.h>
#include <cstdio>
#include <random>
#include <functional>
#include <sstream>
#include <chrono>

std::string NdCalculateUtil::generateMixRandomCode(int count)
{
	const char *content = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, strlen(content) - 1);
	auto dice = std::bind(dis, gen);
	std::ostringstream oss;
	for (int i = 0; i < count; i++) {
		oss << content[dice()];
	}

	return oss.str();
}

std::string NdCalculateUtil::encryptHMac256(const std::string& rawMac, const std::string &macKey)
{
	unsigned char* buffer       = nullptr;
	unsigned int buffer_len		= 0;
	HMacEncode("sha256", macKey.c_str(), macKey.length(), rawMac.c_str(), rawMac.length(), buffer, buffer_len);
	
	std::string rawInfo = (char *)buffer;
	free(buffer);

	//return Base64Encode(std::string((char *)buffer));
	return Base64Encode(rawInfo);
}

std::string NdCalculateUtil::MD5Encrypt(const std::string& src)
{
	unsigned  char saltValue[] = { 0xa3, 0xac, 0xa1, 0xa3, 0x66, 0x64, 0x6a, 0x66, 0x2c, 0x6a, 0x6b, 0x67, 0x66, 0x6b, 0x6c };
	std::string demo = "";
	std::string saltValueStr(reinterpret_cast<const char*>(saltValue), sizeof(saltValue) / sizeof(unsigned char));
	demo = src + saltValueStr;
	MD5_CTX c;
	MD5_Init(&c);
	unsigned char md5[16] = { 0 };
	MD5_Update(&c, demo.c_str(), demo.length());
	MD5_Final(md5, &c);
	char buffer[32] = { 0 };
	for (int i = 0; i < 16; i++)
	{
		sprintf(buffer + i * 2, "%02x", md5[i]);
	}
	return std::string(buffer);
}

std::string NdCalculateUtil::Base64Encode(const char* input, int length, bool withNewLine /* = true */)
{
	BIO *bmem          = NULL;
	BIO *b64           = NULL;
	BUF_MEM *bPtr      = NULL;
	b64                = BIO_new(BIO_f_base64());
	if (!withNewLine)
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bmem               = BIO_new(BIO_s_mem());
	b64                = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	//BIO_flush(b64);
	BIO_ctrl(b64, BIO_CTRL_FLUSH, 0, NULL);
	
	BIO_get_mem_ptr(b64, &bPtr);

	char *buff         = (char *)malloc(bPtr->length + 1);
	memcpy(buff, bPtr->data, bPtr->length);
	buff[bPtr->length] = 0;

	BIO_free_all(b64);
	std::string encoded_content = buff;
	if (buff)
		free(buff);

	return std::move(encoded_content);
}


std::string NdCalculateUtil::Base64Encode(const std::string& input, bool withNewLine /* = true */)
{
	return Base64Encode(input.c_str(), input.length());
}

std::string NdCalculateUtil::RSAEncrypt(const std::string& modules, const std::string& exponent, const std::string& src)
{
	RSA* rsa                   = RSA_new();
	BIGNUM *m                  = BN_new();
	BIGNUM *e                  = BN_new();
	const char *modules_str    = modules.c_str();
	BN_hex2bn(&m, modules_str);
	BN_set_word(e, RSA_F4);
	rsa->n                     = BN_new();
	BN_copy(rsa->n, m);
	rsa->e                     = BN_new();
	BN_copy(rsa->e, e);
	int maxSize                = RSA_size(rsa);

	// unsigend char* encypted = new unsigned char[MAX_SIZE];
	unsigned char*	encrypted  = (unsigned char *)malloc(maxSize);
	memset(encrypted, 0, maxSize * sizeof(unsigned char));
	int bufferSize             = RSA_public_encrypt(src.size(), (unsigned char *)src.c_str(), encrypted, rsa, RSA_PKCS1_PADDING);
	if (bufferSize == -1) {
		if (encrypted)
			free(encrypted);
		RSA_free(rsa);
		return std::string("");
	}

	char buffer[1024]          = { 0 };
	/*
	for (int i                 = 0; i < maxSize; i++) {
		sprintf(buffer + i * 2, "%02x", *encrypted);
		encrypted++;
	}
	*/
	unsigned char* tmp = encrypted;
	for (int i = 0; i < maxSize; i++) {
		sprintf(buffer + i * 2, "%02x", *tmp);
		tmp++;
	}
	if (encrypted)
		free(encrypted);
	if (m)
		BN_free(m);
	if (e)
		BN_free(e);
	RSA_free(rsa);
	
	
	return std::string(buffer);
}

std::string NdCalculateUtil::Base64Decode(char *input, int length, bool withNewLine /* = true */)
{
	BIO *b64 = NULL;
	BIO *bmem = NULL;
	char *buffer = (char *)malloc(length);
	memset(buffer, 0, length);

	b64 = BIO_new(BIO_f_base64());
	if (!withNewLine)
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bmem = BIO_new_mem_buf(input, length);
	bmem = BIO_push(b64, bmem);
	BIO_read(bmem, buffer, length);

	BIO_free_all(b64);

	return std::string(buffer);
}

std::string NdCalculateUtil::Base64Decode(const std::string& input, bool withNewLine /* = true */)
{
	return Base64Decode((char *)input.c_str(), input.size(), withNewLine);
}

bool NdCalculateUtil::HMacEncode(const char* algo, const char* key, unsigned int key_length, const char* input, unsigned int input_length, unsigned char * &output, unsigned int &output_length)
{
	const EVP_MD* engine = NULL;
	if (strcasecmp("sha512", algo) == 0) {
		engine = EVP_sha512();
	}
	else if (strcasecmp("sha256", algo) == 0) {
		engine = EVP_sha256();
	}
	else if (strcasecmp("sha1", algo) == 0) {
		engine = EVP_sha1();
	}
	else if (strcasecmp("md5", algo) == 0) {
		engine = EVP_md5();
	}
	else if (strcasecmp("sha224", algo) == 0) {
		engine = EVP_sha224();
	}
	else if (strcasecmp("sha384", algo) == 0) {
		engine = EVP_sha384();
	}
	else if (strcasecmp("sha", algo) == 0) {
		engine = EVP_sha();
	}
	else {
		return false;
	}

	output = (unsigned char *)malloc(EVP_MAX_MD_SIZE);
	memset(output, 0, EVP_MAX_MD_SIZE);
	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, strlen(key), engine, NULL);
	HMAC_Update(&ctx, (unsigned char*)input, strlen(input));

	HMAC_Final(&ctx, output, &output_length);
	HMAC_CTX_cleanup(&ctx);

	return true;
}

std::vector<std::string> NdCalculateUtil::SplitString(std::string& str, const std::string& pattern)
{
	std::string::size_type      pos;
	std::vector<std::string>    result;
	str += pattern;             //À©Õ¹×Ö·û´®,·½±ã²Ù×÷
	unsigned int size = str.size();

	for (unsigned int i = 0; i < size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

std::string NdCalculateUtil::getNowMilliSeconds()
{
	time_t t				= time(NULL);
	char timeInterval[32]	= { 0 };
	std::sprintf(timeInterval, "%ld356", t);
	return std::string(timeInterval);
}


bool NdCalculateUtil::ContainString(const std::string &strBase, const std::string &subFind)
{
	std::string::size_type  pos = 0;
	pos = strBase.find(subFind, pos);
	if (pos != std::string::npos)
		return true;
	return false;
}

std::string NdCalculateUtil::getCurrentSystemTime()
{
	auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* ptm = std::localtime(&tt);
	char data[32] = { 0 };
	std::sprintf(data, "%d-%02d-%02d", (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday);

	return std::move(std::string(data));
}
