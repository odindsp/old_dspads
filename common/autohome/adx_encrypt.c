#include "adx_encrypt.h"

#include <openssl/hmac.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
int urldecode(char *str, int len)
{
	char *dest = str;
	char *data = str;
	int value;
	unsigned char  c;
	while (len--)
	{
		if (*data == '+')
		{
			*dest = ' ';
		}
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))&& isxdigit((int) *(data + 2)))
		{
			c = ((unsigned char *)(data+1))[0];
			if (isupper(c))
				c = tolower(c);
			value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
			c = ((unsigned char *)(data+1))[1];
			if (isupper(c))
				c = tolower(c);
			value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
			*dest = (char)value ;
			data += 2;
			len -= 2;
		}
		else
		{
			*dest = *data;
		}
		++data;
		++dest;
	}
	*dest = '\0';
	return (dest - str);
}

void hmac_sha1_n(adx_str_t * ekey, adx_str_t * data,
                        size_t n, char * result)
{
    // 20 bytes at least
    u_char tmp[20];
    uint32_t len = 0;

    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    // Using sha1 hash engine here.
    // You may use other hash engines. e.g EVP_md5(), EVP_sha224, EVP_sha512, etc
    HMAC_Init_ex(&ctx, ekey->str, ekey->len, EVP_sha1(), NULL);
    HMAC_Update(&ctx, data->str, data->len);
    HMAC_Final(&ctx, tmp, &len);
    HMAC_CTX_cleanup(&ctx);

    memcpy(result, tmp, n);
}

static void
encode_base64_internal(adx_str_t *dst, adx_str_t *src, const u_char *basis,
            uint32_t padding)
{
    u_char         *d, *s;
    size_t          len;

    len = src->len;
    s = src->str;
    d = dst->str;

    while (len > 2) {
        *d++ = basis[(s[0] >> 2) & 0x3f];
        *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len) {
        *d++ = basis[(s[0] >> 2) & 0x3f];

        if (len == 1) {
            *d++ = basis[(s[0] & 3) << 4];
            if (padding) {
                *d++ = '=';
            }

        } else {
            *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis[(s[1] & 0x0f) << 2];
        }

        if (padding) {
            *d++ = '=';
        }
    }

    dst->len = d - dst->str;
}

static int 
decode_base64_internal(adx_str_t *dst, adx_str_t *src, const u_char *basis)
{
    size_t          len;
    u_char         *d, *s;

    for (len = 0; len < src->len; len++) {
        if (src->str[len] == '=') {
            break;
        }

        if (basis[src->str[len]] == 77) {
            return -1;
        }
    }

    if (len % 4 == 1) {
        return -1;
    }

    s = src->str;
    d = dst->str;

    while (len > 3) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
        *d++ = (u_char) (basis[s[2]] << 6 | basis[s[3]]);

        s += 4;
        len -= 4;
    }

    if (len > 1) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
    }

    if (len > 2) {
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
    }

    dst->len = d - dst->str;

    return 0;
}


void encode_base64(adx_str_t *dst, adx_str_t *src)
{
    static u_char   basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    encode_base64_internal(dst, src, basis64, 1);
}

int decode_base64(adx_str_t *dst, adx_str_t *src)
{
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return decode_base64_internal(dst, src, basis64);
}

int64_t hl64ton(int64_t host)   
{   
    int64_t ret = 0;   
    int32_t high,low;

    low = host & 0xFFFFFFFF;
    high = (host >> 32) & 0xFFFFFFFF;
    low = htonl(low);   
    high = htonl(high);   

    ret   =   low;
    ret   <<= 32;   
    ret   |=   high;   

    return   ret;   
}

int64_t ntohl64(int64_t host)   
{   
    int64_t   ret = 0;   

    int32_t high,low;

    low   =   host & 0xFFFFFFFF;
    high   =  (host >> 32) & 0xFFFFFFFF;
    low   =   ntohl(low);   
    high   =   ntohl(high);   

    ret   =   low;
    ret   <<= 32;   
    ret   |=   high;   
    
    return   ret;   
}

void encrypt_price(const char * iv, const char * ekey, const char * ikey,
                    int64_t win_cpm_price, adx_str_t * result)
{
    char pad[sizeof(int64_t)] = {0};
    char price[sizeof(int64_t)] = {0};

    size_t iv_len = strlen(iv);
    size_t ekey_len = strlen(ekey);
    size_t ikey_len = strlen(ikey);

    adx_str_t key_str, data_str;
    adx_str_set(key_str, ekey, ekey_len);
    adx_str_set(data_str, iv, iv_len);
    
    hmac_sha1_n(&key_str, &data_str, sizeof(pad), pad);
    
    int64_t price_bedian = hl64ton(win_cpm_price);
    memcpy(price, &price_bedian, sizeof(int64_t));

    char c_enc_price[sizeof(int64_t)] = {0}; 
    for (int i = 0; i != sizeof(int64_t); ++i) {
        c_enc_price[i] = pad[i] ^ price[i];
    }
   
    adx_str_set(key_str, ikey, ikey_len);
    char c_price_iv[sizeof(int64_t) + iv_len];
    memcpy(c_price_iv, &price_bedian, sizeof(int64_t));
    memcpy(c_price_iv+sizeof(int64_t), iv, iv_len);
    adx_str_set(data_str, c_price_iv, sizeof(c_price_iv));

    char signature[4] = {0};
    hmac_sha1_n(&key_str, &data_str, sizeof(signature), signature); 
    
    char c_msg[sizeof(signature) + sizeof(int64_t) + iv_len];
    memcpy(c_msg, iv, iv_len);
    memcpy(c_msg+iv_len, c_enc_price ,sizeof(int64_t));
    memcpy(c_msg+iv_len+sizeof(int64_t), signature, sizeof(signature));
    adx_str_set(data_str, c_msg, sizeof(c_msg));

    if (data_str.len == ENCRYPT_PRICE_LENGTH) {    
        encode_base64(result, &data_str);
    }
}

static int split_encrypt(adx_str_t * encrypt_price, adx_str_t * iv, 
                    adx_str_t * enc_price, adx_str_t * signature)
{
    if (encrypt_price->len != 28) {
        return -1;
    }

    adx_str_set(*iv, encrypt_price->str, 16);
    adx_str_set(*enc_price, encrypt_price->str + 16, 8);
    adx_str_set(*signature, encrypt_price->str + 24, 4);
    
    return 0;
}


int decrypt_price(const char * bytes_encrypt_price, const char * ekey, 
                   const char * ikey, int64_t * price)
{
    adx_str_t * e_price = NULL;
    size_t bytes_len = strlen(bytes_encrypt_price);
    size_t tmp_len = base64_decoded_length(bytes_len);
    adx_str_new(e_price, tmp_len);

    adx_str_t data;
    adx_str_set(data, bytes_encrypt_price, bytes_len);
    int res = decode_base64(e_price, &data);

    if (res) {
        adx_str_free(e_price);
        return -1;
    }

    adx_str_t iv, enc_price, signature;
    res = split_encrypt(e_price, &iv, &enc_price, &signature);
    
    if (res) {
        adx_str_free(e_price);
        return -2;
    }
    
    char pad[sizeof(int64_t)] = {0};

    adx_str_t key;
    adx_str_set(key, ekey, strlen(ekey));
    hmac_sha1_n(&key, &iv, sizeof(pad), pad); // take first 8 bytes

    char c_denc_price[sizeof(int64_t)] = {0};
    for (int i = 0; i != sizeof(int64_t); ++i) {
        c_denc_price[i] = enc_price.str[i] ^ pad[i];
    }
    
    char c_signature[4] = {0}; 
    adx_str_set(key, ikey, strlen(ikey));

    char dprice_iv[iv.len + sizeof(c_denc_price)];
    memcpy(dprice_iv, c_denc_price, sizeof(c_denc_price));
    memcpy(dprice_iv+sizeof(c_denc_price), iv.str, iv.len);
    adx_str_set(data, dprice_iv, sizeof(dprice_iv));

    hmac_sha1_n(&key, &data, sizeof(c_signature), c_signature); // --take first 4 bytes
    
    adx_str_free(e_price);

    int sign_cmp = memcmp(c_signature, signature.str, signature.len);
    if (!sign_cmp) {
        int64_t * p_price = (int64_t *)c_denc_price;
        *price = ntohl64(*p_price);
        return 0;
    } else {
        return -3;
    }
}
int DecodeWinningPrice(char *encodedprice, double *value)
{
        char * ekey = "LDddztxx0MWPzQOW";
        char * ikey = "Q7SHR27zfD6UAZLy";
        char * msg = encodedprice;
        int64_t price = 0;		

	int decode_len = urldecode(msg, strlen(msg));
        
        for(int i = 0; i < decode_len; ++i)
        {
               if (msg[i] == ' ')
                  msg[i] = '+';

        }
	if(decrypt_price(msg, ekey, ikey, &price))
	{
		printf("decryption failed!!\n");
		*value = 0;
		return 1;
	}

	//printf("price is :%ld\n",price);
	*value = price ;
	return 0;
}
/*
int main()
{
    char msg[] = "bP%2FERerNT5aS19HYOq2XUoF7cUMTUoZPOWIenQ%3D%3D";
        //char *msg = "YWFhYWFhYWFiYmJiYmJiYlVpmWnPXH0mEuUCgg==";
	double value = 0;
	int decode_len = urldecode(msg, strlen(msg));
	int ret = DecodeWinningPrice(msg,&value);
	//cout << "price is :" <<*value <<endl;
	if(!ret)
	{
		printf("decode price success!\n");
	}
	printf("price is %lf\n",value);
	return 0;
}
*/
