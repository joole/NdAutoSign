#ifndef NDCALCULATEUTIL_H
#define NDCALCULATEUTIL_H

#include <string>
#include <vector>

class NdCalculateUtil
{
public:
	static std::string generateMixRandomCode(int count);
	static std::string encryptHMac256(const std::string& rawMac, const std::string &macKey);
	static std::string MD5Encrypt(const std::string& src);
	static std::string RSAEncrypt(const std::string & modules, const std::string & exponent, const std::string & src);
	static std::string Base64Encode(const std::string& input, bool withNewLine = true);
	static std::string Base64Encode(const char* input, int length, bool withNewLine = true);
	static std::string Base64Decode(char *input, int lenght, bool withNewLine = true);
	static std::string Base64Decode(const std::string& input, bool withNewLine = true);
		
		
		
	static bool HMacEncode(const char* algo, const char* key, unsigned int key_length, const char* input, unsigned int input_length, unsigned char * &output, unsigned int &output_length);
	// ×Ö·û´®´¦Àí
	static std::vector<std::string> SplitString(std::string& str, const std::string& pattern);
	static bool ContainString(const std::string &strBase, const std::string &subFind);

	static std::string getNowMilliSeconds();

	static std::string getCurrentSystemTime();
private:
	NdCalculateUtil() = delete;
	~NdCalculateUtil() = delete;
};

#endif //NDCALCULATEUTIL_H