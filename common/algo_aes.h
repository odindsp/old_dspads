#ifndef ALGO_AES_H
#define ALGO_AES_H
//aes-256-ecb-pkcs5 加密
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, 
                unsigned char *iv, unsigned char *ciphertext);  
// 解密
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,  
		unsigned char *iv, unsigned char *plaintext);  
#endif


