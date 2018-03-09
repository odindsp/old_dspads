#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "base64.h"
#include "decode.h"
using namespace std;
void handleErrors(void)
{
	  ERR_print_errors_fp(stderr);
	  abort();
}

std::string add_padding(const std::string& b64_string) 
{
    if (b64_string.size() % 4 == 3) {
        return b64_string + "=";
    } else if (b64_string.size() % 4 == 2) {
        return b64_string + "==";
    }
    return b64_string;
}

string web_safe_base64_decode(const std::string& encoded)
{
    /* convert from web safe -> normal base64.*/
    string normal_encoded = encoded;
 /*   size_t index = string::npos;
    while ((index = normal_encoded.find_first_of('-', index + 1)) != string::npos) {
        normal_encoded[index] = '+';
    }
    index = string::npos;
    while ((index = normal_encoded.find_first_of('_', index + 1)) != string::npos) {
        normal_encoded[index] = '/';
    }*/

    const int kOutputBufferSize = 256;
    char output[kOutputBufferSize];

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bio = BIO_new_mem_buf(const_cast<char*>(normal_encoded.data()),
                             normal_encoded.length());
    bio = BIO_push(b64, bio);
    int out_length = BIO_read(bio, output, kOutputBufferSize);
    BIO_free_all(bio);

//    cout<<"out_length:"<<out_length<<endl; 
    return string(output, out_length);
}

string web_safe_base64_encode(const std::string& encoded)
{
    /* convert from web safe -> normal base64.*/
    string normal_encoded = encoded;
/*    size_t index = string::npos;
    while ((index = normal_encoded.find_first_of('+', index + 1)) != string::npos) {
        normal_encoded[index] = '-';
    }
    index = string::npos;
    while ((index = normal_encoded.find_first_of('/', index + 1)) != string::npos) {
        normal_encoded[index] = '_';
    }
*/
    return string(base64_encode(normal_encoded.c_str()));
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
		  unsigned char *iv, unsigned char *ciphertext)
{
	  EVP_CIPHER_CTX *ctx;

	  int len;

	  int ciphertext_len;

	  /* Create and initialise the context */
	  if(!(ctx = EVP_CIPHER_CTX_new())) 
		handleErrors();

          /* Initialise the encryption operation. IMPORTANT - ensure you use a key
          *  and IV size appropriate for your cipher
          *  In this example we are using 256 bit AES (i.e. a 256 bit key). The 
	  *  IV size for *most* modes is the same as the block size. For AES this
          *  is 128 bits */
	  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, iv))
	        handleErrors();

	   /* Provide the message to be encrypted, and obtain the encrypted output.
	   *   EVP_EncryptUpdate can be called multiple times if necessary*/
	  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
                handleErrors();
	  ciphertext_len = len;

          /* Finalise the encryption. Further ciphertext bytes may be written at
	  *    * this stage.*/
	  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) 
		  handleErrors();
	 ciphertext_len += len;

	 /* Clean up */
         EVP_CIPHER_CTX_free(ctx);

         return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
		  unsigned char *iv, unsigned char *plaintext)
{
	  EVP_CIPHER_CTX *ctx;

	  int len;

	  int plaintext_len = 0;

	  /* Create and initialise the context */
	  if(!(ctx = EVP_CIPHER_CTX_new())) 
		  return plaintext_len;
          /* Initialise the decryption operation. IMPORTANT - ensure you use a key
	  *  and IV size appropriate for your cipher
	  *  In this example we are using 256 bit AES (i.e. a 256 bit key). The
          *  IV size for *most* modes is the same as the block size. For AES this
		  *  is 128 bits */
	  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, iv))
	  {
		  goto exit;
	  }

          /* Provide the message to be decrypted, and obtain the plaintext output.
	  *  EVP_DecryptUpdate can be called multiple times if necessary*/
	  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
	  {
		  goto exit;
	  }
	  plaintext_len = len;
	  
          /* Finalise the decryption. Further plaintext bytes may be written at
	  *  this stage.*/
	  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) 
	  {
		  goto exit;
	  }
	  plaintext_len += len;
exit:
           /* Clean up */
      EVP_CIPHER_CTX_free(ctx);

      return plaintext_len;
}

int DecodeWinningPrice(char *sourceStr, double *price)
{
	int errcode = 0;
	unsigned char key[]={0x06, 0x61 ,0x28 ,0x50 ,0x37 ,0x71, 0xdd ,0x25 
		,0x17 ,0x88 ,0xe2 ,0x81 ,0x64 ,0x62 ,0x13 ,0x39 };

	unsigned char *iv = NULL;
	string decode_str = "";

	int decryptedtext_len = 0;
	std::string value = string(sourceStr);
	std::string padded = add_padding(value);	
	decode_str = web_safe_base64_decode(padded);
	
	unsigned char *decryptedtext = (unsigned char *)malloc(2048);

	decryptedtext_len = decrypt((unsigned char *)decode_str.c_str(), decode_str.size(), key, iv, decryptedtext);
	if (decryptedtext_len != 0)
	{
		decryptedtext[decryptedtext_len] = '\0';
		*price = atof((const char*)decryptedtext);
	}
	else
	{
		errcode = 1;
		goto exit;
	}

exit:
    if(decryptedtext != NULL)
        free(decryptedtext);
	
	return errcode;
}

//int main()
//{
//	
//    char *sourceStr = "LfaKzQKB7VjTpHd8eK6yvo0KKpQtCW4MLhUEFT+Y524=";
//	double price = 0.0;
//	
//	DecodeWinningPrice(sourceStr, &price);
//	printf("price:%f\n", price);
//
//	return 0;
//}