/**
  AES encryption/decryption demo program using OpenSSL EVP apis
  gcc -Wall openssl_aes.c -lcrypto

  this is public domain code.

  Saju Pillai (saju.pillai@gmail.com)
**/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/evp.h>
#define AES_BLOCK_SIZE 256
/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx,
             EVP_CIPHER_CTX *d_ctx)
{
  int i, nrounds = 5;
  unsigned char key[32], iv[32];

  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
   * nrounds is the number of times the we hash the material. More rounds are more secure but
   * slower.
   */
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }
  
  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  return 0;
}
/*
 * function aes_simple_init
 * 简单加密方式，使用128位的AES加密，ECB模式，初始向量为NULL
 * 输入的key为16字节的二进制密钥
 *
 */
int aes_simple_init(unsigned char* key, EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx){

    EVP_CIPHER_CTX_init(e_ctx);
    EVP_EncryptInit_ex(e_ctx, EVP_aes_128_ecb(), NULL, key, NULL);
    EVP_CIPHER_CTX_init(d_ctx);
    EVP_DecryptInit_ex(d_ctx, EVP_aes_128_ecb(), NULL, key, NULL);
    return 0;
}

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
  /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
  int c_len = *len + AES_BLOCK_SIZE - 1, f_len = 0;
  unsigned char *ciphertext = (unsigned char *)malloc(c_len);

  /* allows reusing of 'e' for multiple encryption cycles */
  if(!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL)){
    printf("ERROR in EVP_EncryptInit_ex \n");
    return NULL;
  }

  /* update ciphertext, c_len is filled with the length of ciphertext generated,
    *len is the size of plaintext in bytes */
  if(!EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len)){
    printf("ERROR in EVP_EncryptUpdate \n");
    return NULL;
  }

  /* update ciphertext with the final remaining bytes */
  if(!EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len)){
    printf("ERROR in EVP_EncryptFinal_ex \n");
    return NULL;
  }

  *len = c_len + f_len;
  return ciphertext;
}

/*
 * Decrypt *len bytes of ciphertext
 */
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
  /* plaintext will always be equal to or lesser than length of ciphertext*/
  int p_len = *len, f_len = 0;
  unsigned char *plaintext = (unsigned char *)malloc(p_len);

  if(!EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL)){
    printf("ERROR in EVP_DecryptInit_ex \n");
    return NULL;
  }

  if(!EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len)){
    printf("ERROR in EVP_DecryptUpdate\n");
    return NULL;
  }

  if(!EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len)){
    printf("ERROR in EVP_DecryptFinal_ex\n");
    return NULL;
  }

  *len = p_len + f_len;
  return plaintext;
}
/*
 * 使用AES-256-CBC加密算法加密给定的字符串
 * 返回加密的字符串
 * @param in
 * 输入的明文字符串
 * @param len
 * 输入的明文字符串的长度, int *len, 返回时*len 被赋值为加密后密文的长度
 * @param key_data
 * 需要输入的密码
 * @return
 * 返回由加密函数分配的一个字符串指针，需要使用后释放
 */
unsigned char* aes_256_encrypt(const char* in, int *len, const char* key_data)
{
    if (!in || !len || !key_data)
        return NULL;

    EVP_CIPHER_CTX en, de;
    int key_data_len;

    key_data_len = strlen(key_data);

    unsigned char salt[] = {1,2,3,4,5,6,7,8};

    unsigned char *ciphertext;


    /* gen key and iv. init the cipher ctx object */
    if (aes_init((unsigned char*)key_data, key_data_len, salt, &en, &de)) {
        printf("Couldn't initialize AES cipher\n");
        return NULL;
    }

    // 此处会修改len的值

    ciphertext = aes_encrypt(&en, (unsigned char *)in, len);
    EVP_CIPHER_CTX_cleanup(&de);
    EVP_CIPHER_CTX_cleanup(&en);

    return ciphertext;

}
/*
 * AES-256-CBC 解密函数
 * @param in
 * 输入的密文
 * @param len
 * 密文的长度, 函数执行完之后会修改len的值，为明文的长度
 * @key_data
 * 输入的密码
 * @return 
 * 返回明文字符串，需要free
 */
char *aes_256_decrypt(const unsigned char* in, int *len, const char* key_data)
{

    if (!in || !len || !key_data)
        return NULL;

    EVP_CIPHER_CTX en, de;
    int key_data_len;
    key_data_len = strlen(key_data);

    unsigned char salt[] = {1,2,3,4,5,6,7,8};

    char *plaintext;
   /* gen key and iv. init the cipher ctx object */
    if (aes_init((unsigned char*)key_data, key_data_len, salt, &en, &de)) {
        printf("Couldn't initialize AES cipher\n");
        return NULL;
    }

    plaintext = (char *)aes_decrypt(&de, (unsigned char*)in, len);


    EVP_CIPHER_CTX_cleanup(&de);
    EVP_CIPHER_CTX_cleanup(&en);

    return plaintext;

}
/*
 * 使用AES-128-ECB加密算法加密给定的字符串
 * 返回加密的字符串
 * @param in
 * 输入的明文字符串
 * @param len
 * 输入的明文字符串的长度, int *len, 返回时*len 被赋值为加密后密文的长度
 * @param key_data
 * 需要输入的密码
 * @return
 * 返回由加密函数分配的一个字符串指针，需要使用后释放
 */
unsigned char* aes_128_ecb_encrypt(const char* in, int *len, const char* key_data)
{
    if (!in || !len || !key_data)
        return NULL;

    EVP_CIPHER_CTX en, de;

    unsigned char *ciphertext;

    aes_simple_init((unsigned char*)key_data, &en, &de);
    // 此处会修改len的值

    ciphertext = aes_encrypt(&en, (unsigned char *)in, len);
    EVP_CIPHER_CTX_cleanup(&de);
    EVP_CIPHER_CTX_cleanup(&en);

    return ciphertext;

}
/*
 * AES-128-ECB 解密函数
 * @param in
 * 输入的密文
 * @param len
 * 密文的长度, 函数执行完之后会修改len的值，为明文的长度
 * @key_data
 * 输入的密码
 * @return 
 * 返回明文字符串，需要free
 */
char *aes_128_ecb_decrypt(const unsigned char* in, int *len, const char* key_data)
{

    if (!in || !len || !key_data)
        return NULL;

    EVP_CIPHER_CTX en, de;

    char *plaintext;
    aes_simple_init((unsigned char*)key_data, &en, &de);
   
    plaintext = (char *)aes_decrypt(&de, (unsigned char*)in, len);

    EVP_CIPHER_CTX_cleanup(&de);
    EVP_CIPHER_CTX_cleanup(&en);

    return plaintext;

}

/*
int main(int argc, char **argv)
{
  return 0;
}
*/

