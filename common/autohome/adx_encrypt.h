#ifndef _ADX_UTIL_ENCRYPT_H_
#define _ADX_UTIL_ENCRYPT_H_

#include <stddef.h>
#include <stdint.h>

typedef unsigned char u_char;

typedef struct {
    u_char   * str;
    size_t   len;
} adx_str_t;

#define adx_str_new(adx_str, str_len) \
          (adx_str) = (adx_str_t*)malloc(sizeof(adx_str_t));           \
          (adx_str)->str = (u_char*)malloc(str_len); (adx_str)->len = str_len

#define adx_str_set(adx_str, c_str, c_len) \
          (adx_str).str = (c_str); (adx_str).len = (c_len); 

#define adx_str_free(adx_str)     \
          free((adx_str)->str); (adx_str)->len = 0; free(adx_str)

#define ENCRYPT_PRICE_LENGTH    (20 + sizeof(int64_t))

#define base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define base64_decoded_length(len)  (((len + 3) / 4) * 3)

void hmac_sha1_n(adx_str_t * ekey, adx_str_t * data, 
                        size_t n, char * result);

void encode_base64(adx_str_t *dst, adx_str_t *src);

int decode_base64(adx_str_t *dst, adx_str_t *src);
                                         
int64_t hl64ton(int64_t host);

int64_t ntohl64(int64_t host);

void encrypt_price(const char * iv, const char * ekey, const char * ikey,
                    int64_t win_cpm_price, adx_str_t * result);

int decrypt_price(const char * bytes_encrypt_price, const char * ekey, 
                   const char * ikey, int64_t * price);

int DecodeWinningPrice(char *encodedprice, double *value);
#endif // _ADX_UTIL_ENCRYPT_H_
