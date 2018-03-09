#include <string.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <openssl/aes.h>  
#include <openssl/evp.h>  
#include <iostream>

#include "decode.h"
#include "settlement.pb.h"
using namespace std;

#define EVP_DES_CBC EVP_aes_128_ecb();  
#define MAX_CHAR_SIZE 512  
typedef unsigned char	uint8_t;
char encryption_key_v[] = {
        0xd8, 0xcf, 0x05, 0x39,
        0x17, 0xe8, 0x2a, 0xc7,
        0x57, 0x9f, 0x61, 0x90,
        0x32, 0x68, 0x4b, 0x5a
};

char outs[] = {
        0x40, 0xdd, 0x88, 0xe1,
        0x15, 0xaa, 0xb3, 0x4f,
        0xfa, 0x94, 0x9d, 0xfb,
        0x24, 0x5e, 0x8e, 0x97
};

#define kPriceCipherSize 16

static const std::string ENCRYPTION_KEY(encryption_key_v, 16);
static const std::string INTEGRITY_KEY(outs, 16);

const char kPlaceHolder = '!';

const string kBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz0123456789_-";

string internal_base64_encode(const string& base64_chars,
							const char place_holder,
							const unsigned char* bytes_to_encode,
							size_t in_len)
{
	string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) 
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) 
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

		for(i = 0; (i <4) ; i++)
			ret += base64_chars[char_array_4[i]];
		
		i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++)
		char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += place_holder;
}

	return ret;
}

string base64_encode_q(const unsigned char* bytes_to_encode, size_t in_len) {
	return internal_base64_encode(kBase64Chars,
                                  kPlaceHolder, bytes_to_encode, in_len);
}

inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '_') || (c == '-'));
}

typedef bool (*func_ptr)(unsigned char);

string internal_base64_decode(const string& base64_chars,
                              func_ptr is_base64,
                              const char place_holder,
                              const string& encoded_string) {
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	string ret;

	while (in_len-- && ( encoded_string[in_] != place_holder) &&
          is_base64(encoded_string[in_])) 
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
			char_array_4[i] = base64_chars.find(char_array_4[i]);
	
			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;

		//	printf("decode ret:%s\n", ret.c_str());
		}
	}

	if (i) {
	for (j = i; j <4; j++)
		char_array_4[j] = 0;

	for (j = 0; j <4; j++)
		char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
	
	for (j = 0; (j < i - 1); j++) 
		ret += char_array_3[j];
  }

	return ret;
}

string base64_decode_q(const string& encoded_string) {
    return internal_base64_decode(kBase64Chars,
            is_base64, kPlaceHolder, encoded_string);
}

const std::string DecryptPrice(const std::string& cipher, const std::string& key)  {
	char buffer[kPriceCipherSize];
	int price_length;
	EVP_CIPHER_CTX ctx;
  
	EVP_DecryptInit(&ctx,
                  EVP_aes_128_ecb(), reinterpret_cast<const uint8_t*>(key.data()),
                  NULL);
	EVP_DecryptUpdate(&ctx,
                    reinterpret_cast<uint8_t*>(buffer),
                    &price_length,
                    reinterpret_cast<const uint8_t*>(cipher.data()),
                    kPriceCipherSize);
	EVP_DecryptFinal(&ctx,
                   reinterpret_cast<uint8_t*>(buffer),
                   &price_length);

	EVP_CIPHER_CTX_cleanup(&ctx);

	return std::string(buffer, price_length);
}

const std::string DecryptPrice(unsigned char *cipher, const std::string& key)  {
	char buffer[kPriceCipherSize];
	int price_length;
	EVP_CIPHER_CTX ctx;
	
	EVP_DecryptInit(&ctx,
                  EVP_aes_128_ecb(), reinterpret_cast<const uint8_t*>(key.data()),
                  NULL);
	EVP_DecryptUpdate(&ctx,
                    reinterpret_cast<uint8_t*>(buffer),
                    &price_length,
                    cipher,
                    kPriceCipherSize);
	EVP_DecryptFinal(&ctx,
                   reinterpret_cast<uint8_t*>(buffer),
                   &price_length);

	EVP_CIPHER_CTX_cleanup(&ctx);

	return std::string(buffer, price_length);
}

int String2Bytes(char* szSrc, unsigned char* pDst)     
{
    int iLen = 0;

    if ((szSrc == NULL) || (pDst == NULL))
	return 0;

    iLen = strlen(szSrc);
    printf("ilen:%d\n", iLen);
    if (iLen <= 0 || iLen%2 != 0)
        return 0;
    
    iLen /= 2;

    for (int i = 0; i < iLen; i++)  
    {  
        int iVal = 0;  
        unsigned char *pSrcTemp = (unsigned char *)szSrc + i*2;  
        sscanf((char *)pSrcTemp, "%02x", &iVal);  
        pDst[i] = (unsigned char)iVal;
    }  
    
    for(int i = 0;i < iLen; i++) //print 
       printf("%.02x", pDst[i]); 

    printf("\n"); 
    return iLen;  
}

int DecodeWinningPrice(char *encodedprice, double *value)
{
	price::Settlement settle;
	string output = "";

	char ch[3] = "";
	char pOut[16] = "";	
	unsigned char pDst[17] = "", pDsts[17] = "";
	
/*	char *outputdata = "";
	settle.set_version(12345);
	settle.set_auth("1234567");

	int ilen = 0;
	//2ac078242bb15dfe47216b447ebb9c41 = 1234
	//a516ea5455a10efc7f01af26d9ef26a3 = 3000
	//3e03eb48d6b139ad887d08c8f28a8dd7 = 12.09
	ilen = String2Bytes("2ac078242bb15dfe47216b447ebb9c41", pDsts);

	settle.set_price((char *)pDsts);

	int lens = 0;
	lens = settle.ByteSize();
	outputdata = (char *)calloc(1, sizeof(char) * (lens + 1));
	
	settle.SerializeToArray(outputdata, lens);

	std::string dd = base64_encode_q((unsigned char *)outputdata, strlen(outputdata));
	printf("encode:%s\n", dd.c_str());
*/
	price::Settlement desettle;
	bool parsesuccess = desettle.ParseFromArray(base64_decode_q(string(encodedprice)).c_str(), base64_decode_q(string(encodedprice)).size());
	
//	std::string aa = base64_decode_q(dd);
//	bool parsesuccess = desettle.ParseFromArray(aa.c_str(), aa.size());
	//printf("version:%d\n", desettle.version());
	//printf("price size:%d\n", desettle.price().size());
	for(int i = 0;i < desettle.price().size(); i++)
	{
		sprintf(ch, "%.02x", reinterpret_cast<const uint8_t*>(desettle.price().data())[i]);
		output += ch;
	}

	int len = 0;

	char *ss = new char[33];
	strcpy(ss, output.c_str());

	len = String2Bytes(ss, pDst);

	//printf("pDst length:%d\n", strlen((char *)pDst));
	
	std::string sprice = DecryptPrice(pDst, ENCRYPTION_KEY);

	//printf("sprice:%s\n", sprice.c_str());
	
	*value = atof(sprice.c_str());	
	if (ss != NULL)
		free(ss);
	return 0;
}
/*int main(int argc, char **argv)
{
	double price = 0.0;
	
	DecodeWinningPrice("CPPiKxIQQN2I4RWqs0-6lJ37JF6Olw!!", &price);
	
	printf("price:%f\n", price);
	
    return 0;  
}*/
