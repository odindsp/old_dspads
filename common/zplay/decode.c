/* 
 * File:   price.h
 * Author: weihaibin
 *
 * Created on 2015年12月2日, 下午4:10
 */

#include "base64.h"
#include "hex.h"
#include "hmac_sha1.h"
#include "decode.h"

#define TIME_STAMP_LEN  4
#define ENC_PRICE_LEN   8
#define SIG_LEN         4

char* xor_bytes(char* b1,char* b2,size_t len){
    char* ret = malloc(len);
	if (ret == NULL)
		return NULL;
    for (int i = 0; i < len; i++){
        ret[i] = b1[i] ^ b2[i];
    }
    return ret;
}

double price_decode(const char* dec_key,const char* sign_key,const char* src){
    
	double price = 0;
    char *time_ms_stamp_bytes = NULL;
    char *enc_price = NULL;
    char *sig = NULL;
	size_t n = 0;
	int dec_key_len = 0;
	unsigned char *dec_key_bytes = NULL;
	unsigned char pad[20];
	unsigned char *dec_price = NULL;
	char *q = base64url_decode(src);
    if (q == NULL)
	{
	   goto exit;
	}

	time_ms_stamp_bytes = malloc(TIME_STAMP_LEN);
	if (time_ms_stamp_bytes == NULL)
		goto exit;

    enc_price = malloc(ENC_PRICE_LEN);
	if (enc_price == NULL)
		goto exit;

    sig = malloc(SIG_LEN);
	if (sig == NULL)
		goto exit;

    for (int i = 0; i < TIME_STAMP_LEN; i++){
        time_ms_stamp_bytes[i] = q[n];
        n++;
    }
    for (int i = 0; i < ENC_PRICE_LEN; i++){
        enc_price[i] = q[n];
        n++;
    }
    for (int i = 0; i < SIG_LEN; i++){
        sig[i] = q[n];
        n++;
    }
    
    dec_key_bytes = hex(dec_key,&dec_key_len);
    if (dec_key_bytes == NULL)
		goto exit;


    hmac_sha1(dec_key_bytes,  dec_key_len,time_ms_stamp_bytes, TIME_STAMP_LEN, pad);  
    dec_price = xor_bytes(pad,enc_price,8);

	if (dec_price == NULL)
		goto exit;
    memcpy(&price,dec_price,8);
    
    //校验
 //   int sign_key_bytes_len;
 //   unsigned char* sign_key_bytes = hex(sign_key,&sign_key_bytes_len);
 //   unsigned char* sign_com_bytes = malloc(ENC_PRICE_LEN+TIME_STAMP_LEN);
 //   
	//memcpy(sign_com_bytes,dec_price,ENC_PRICE_LEN);
 //   memcpy(&sign_com_bytes[ENC_PRICE_LEN],time_ms_stamp_bytes,TIME_STAMP_LEN);
 //   
 //   unsigned char* osig = malloc(20);
 //   hmac_sha1(sign_key_bytes,sign_key_bytes_len,sign_com_bytes,ENC_PRICE_LEN+TIME_STAMP_LEN,osig);

exit:
	if(dec_price)
	{
	  free(dec_price);
	  dec_price = NULL;
	}
	if (dec_key_bytes)
	{
	  free(dec_key_bytes);
	  dec_key_bytes = NULL;
	}

	if (time_ms_stamp_bytes)
	{
	  free(time_ms_stamp_bytes);
	  time_ms_stamp_bytes = NULL;
	}

	if (enc_price)
	{
	  free(enc_price);
	  enc_price = NULL;
	}

	if (sig)
	{
	  free(sig);
	  sig = NULL;
	}

	if (q)
	{
	  free(q);
	  q = NULL;
	}
    return price;
}

int DecodeWinningPrice(char *source, double *price)
{
    char *dec_key="3895185f372f4bba9a594fd65e5205f2";   //对接需要修改
    char *sign_key = "da2d9026aa90400cbc9c4db053e65b82"; 

    *price = price_decode(dec_key, sign_key, source);

    if (*price == 0)
		return 1;

//    printf("price:%f\n", *price);
    return 0;
}
