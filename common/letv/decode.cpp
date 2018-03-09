#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include "decode.h"
using namespace std;

extern "C"

#define AESDE_LEN     	1024
#define LETV_TOKEN 		"6d915d5c9a32498480fa4ed45a491afc" //正式上线的token //"6dd6b8b992bf4fee9ba75d0506012cb6" //letv测试 token

typedef unsigned char uchar;

/* Hex string to integer */
//Example:"ac" to 0xac
static int htoi(char *s)
{
    int value;
    int c;

    c = ((uchar*)s)[0];
    if (isupper(c))
    {
        c = tolower(c);
    }
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((uchar*)s)[1];
    if (isupper(c))
    {
        c = tolower(c);
    }
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}

// Standard url decoder
static int urlDecode(char *str, int len)
{
    char *dest = str;
    char *data = str;

    while (len--)
    {
        if (*data == '+')
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2
                && isxdigit((int) *(data + 1))
                && isxdigit((int) *(data + 2)))
        {
            *dest = (char) htoi(data + 1);
            data += 2;
            len -= 2;
        }
        else
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return dest - str;
}

int DecodeWinningPrice(char *sourceStr, double *price)
{
	int err = 0;
	int len = 0;
	char *base64Decode = NULL;
	unsigned char *iv = NULL;

	int decryptedtext_len = 0;
	//ERR_load_crypto_strings();
	//OpenSSL_add_all_algorithms();
	//OPENSSL_config(NULL);	

	unsigned char *decryptedtext = (unsigned char *)malloc(AESDE_LEN);

	len = urlDecode(sourceStr,strlen(sourceStr));
	if(0 == len)
	{
		err = 1;
		goto exit;
	}

	base64Decode = base64_decode(sourceStr);
	if(NULL == base64Decode)
	{
		err = 1;
		goto exit;
	}
	
	decryptedtext_len = decrypt((unsigned char *)base64Decode, strlen(base64Decode), (unsigned char *)LETV_TOKEN, iv, decryptedtext);
	
	if(decryptedtext_len != 0)
	{
		decryptedtext[decryptedtext_len] = '\0';
		*price = atof((const char*)decryptedtext);
	}
	else
	{
		err = 1;
		*price = 0.0;
		goto exit;
	}
		
	
exit:
	//EVP_cleanup();
	//ERR_free_strings();
	if(base64Decode != NULL)
		free(base64Decode);
	
	if(decryptedtext != NULL)
		free(decryptedtext);
	
	return err;
}
