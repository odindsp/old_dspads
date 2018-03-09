/**
 * Copyright (c) 2012 Tanx.com
 * All rights reserved.
 *
 * This is a sample code to show the decode encoded price. 
 * It uses the Base64 decoder and the MD5 in the OpenSSL library.
 */

#include <iostream>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <string.h>
#include "decode.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;

extern "C"

// Calc the length of the decoded 
#define base64DecodedLength(len)  (((len + 3) / 4) * 3)

// Check the endian of the machine
/*static*/ int32_t __endianCheck = 1;
#define IS_BIG_ENDIAN (*(char*)&__endianCheck == 0)

// Reverse by byte
#define SWAB32(x) \
    ((uint32_t)( \
        (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) | \
        (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) | \
        (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) | \
        (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))

#define VERSION_LENGTH 1
#define BIDID_LENGTH 16
#define ENCODEPRICE_LENGTH 4
#define CRC_LENGTH 4
#define KEY_LENGTH 16

#define VERSION_OFFSITE 0
#define BIDID_OFFSITE (VERSION_OFFSITE + VERSION_LENGTH)
#define ENCODEPRICE_OFFSITE (BIDID_OFFSITE + BIDID_LENGTH)
#define CRC_OFFSITE (ENCODEPRICE_OFFSITE + ENCODEPRICE_LENGTH)
#define MAGICTIME_OFFSET 7

typedef unsigned char uchar;

// Key just for test
// Get it from tanx, format like: 
// f7dbeb735b7a07f1cfca79cc1dfe4fa4
static uchar g_key[] = {
    0x13, 0x76, 0x59, 0x77, 0xec, 0x84, 0x52, 0xf5,
    0xa3, 0xc6, 0x56, 0x87, 0xaf, 0x31, 0x58, 0x12
};

int Add(char *encode_price, int32_t *price)
{
    cout<<"11111111111111111111"<<endl;
}

// Hex string to integer
// For example: 'ac' to 0xac
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

// Standard base64 decoder
static int base64Decode(char* src, int srcLen, char* dst, int dstLen)
{
    BIO *bio, *b64 ;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new_mem_buf(src, srcLen);
    bio = BIO_push(b64, bio);
    int dstNewLen = BIO_read(bio, dst, dstLen);
    BIO_free_all(bio);
    return dstNewLen;
}

// Step list for decoding original price
// If this function returns true, real price will return to realPrice 
// variable and bidding time will return to time variable
// If this function returns false, it means format error or checksum
// error, realPrice variable and time variable are invalid
static bool decodePrice(uchar* src, int32_t& realPrice, time_t& time)
{
    // Get version and check
    int v = *src;
    if (v != 1)
    {
        cerr << "version:" << v << "!=1, error" << endl;
        return false;
    }

    // Copy bidid+key
    uchar buf[64];
    memcpy(buf, src + BIDID_OFFSITE, BIDID_LENGTH);
    memcpy(buf + BIDID_LENGTH, g_key, KEY_LENGTH);

    // ctxBuf=MD5(bidid+key)
    uchar ctxBuf[16];
    MD5(buf, 32, ctxBuf);

    // Get settle price
    int32_t price = 0;
    uchar* p1 = (uchar*)&price;
    uchar* p2 = src + ENCODEPRICE_OFFSITE;
    uchar* p3 = ctxBuf;
    for (int i = 0; i < 4; ++i)
    {
        *p1++ = *p2++ ^ *p3++;
    }

    // Big endian needs reverse price by byte
    if (IS_BIG_ENDIAN)
    {
        realPrice = SWAB32(price);
    }
    else
    {
        realPrice = price;
    }

    // Calc crc and compare with src
    // If not match, it is illegal

    // copy(version+bidid+settlePrice+key)
    const int vb = VERSION_LENGTH + BIDID_LENGTH;
    uchar* pbuf = buf;
    memcpy(pbuf, src, vb);// copy version+bidid
    pbuf += vb;

    // Notice: here is price not realPrice
    // More important for big endian !
    memcpy(pbuf, &price, 4);// copy settlePrice
    pbuf += 4;
    memcpy(pbuf, g_key, KEY_LENGTH);// copy key

    // MD5(version+bidid+settlePrice+key)
    MD5(buf, VERSION_LENGTH + BIDID_LENGTH + 4 + KEY_LENGTH, ctxBuf);

    if (0 != memcmp(ctxBuf, src + CRC_OFFSITE, CRC_LENGTH))
    {
        cerr << "checksum error!" << endl;
        return false;
    }

    time = ntohl(*(uint32_t*)(src + MAGICTIME_OFFSET));
    return true;
}

/*
 * More sample string for encoded price:
 *   1. src: AQwKgWQE5lCXia8AAAX5oD%2BUK65TMWRP6A%3D%3D
 *      result:
 *           Decode price ok, price = 15
 *           Bid time is 2012-11-05 17:41:03
 *   2. src: AQtlz3ImkFBRQ5MAAADFkK5AX2YJU4kNig%3D%3D
 *      result:
 *           Decode price ok, price = 45
 *           Bid time is 2012-09-13 10:23:15
 *   3. src: Ad9B18wDhlBWphkAAh1Ok6K1axl8XRHrqg%3D%3D
 *      result:
 *           Decode price ok, price = 126
 *           Bid time is 2012-09-17 12:24:57
 *   4. src: AQCugRQORVBzGnkAAAAAAHtCaywmy1HEwg%3D%3D
 *      result:
 *           Decode price ok, price = 87
 *           Bid time is 2012-10-09 02:24:57
 */
//int main(int argc, char* argv[])
//{
    // The sample string for encoded price
    // result: 
    //    Decode price ok, price = 121
    //    Bid time is 2012-09-29 19:19:33
//    char src[] = "AbuI6wqbQlBm2UUAAAEAAADaQPIFMFCbeQ%3D%3D";

int DecodeWinningPrice(char *src, double *price)
{
    // Standard url decoder
    int len = urlDecode(src, strlen(src));

    // Calc the length of the decoded string
    int origLen = base64DecodedLength(len);
    uchar* orig = new uchar[origLen];

    // Standard BASE64 decoder using ssl library
    origLen = base64Decode(src, len, (char*)orig, origLen);

    // 25 bytes fixed length
    if (origLen != VERSION_LENGTH + BIDID_LENGTH 
                   + ENCODEPRICE_LENGTH + CRC_LENGTH)
    {
        delete [] orig;
        cerr << "base64 decode error!" << endl;
        return 1;
    }

    int32_t pc;
    time_t time;
    if (decodePrice(orig, pc, time))
    {
        struct tm tm;
        localtime_r(&time, &tm);

        cout << "Decode price ok, price = " << pc  << endl;
        printf("Bid time is %04d-%02d-%02d %02d:%02d:%02d\n",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    *price = (double)pc;    

    delete [] orig;
    return 0;
}
