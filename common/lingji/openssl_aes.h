#ifndef _OPENSSL_AES_H_
#define _OPENSSL_AES_H_
/*
 * 256位的AES CBC解密
 */
char *aes_256_decrypt(const unsigned char* in, int *len, const char* key_data);

/*
 * 256位的AES CBC加密
 */
unsigned char* aes_256_encrypt(const char* in, int *len, const char* key_data);

/* 
 * 128位的AES ECB加密
 */
unsigned char* aes_128_ecb_encrypt(const char* in, int *len, const char* key_data);
/*
 * 128位的AES ECB解密
 * t: 是否在解密后的明文末尾增加'\0',默认为不加
 *
 */
char *aes_128_ecb_decrypt(const unsigned char* in, int *len, const char* key_data);
#endif // _OPENSSL_AES_H_
