/**
 * Copyright (c) 2012 Tanx.com
 * All rights reserved.
 *
 * This is a sample code to show the decode encoded deviceId. 
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

// Calc the length of the decoded  
#define base64DecodedLength(len)  (((len + 3) / 4) * 3)

#define VERSION_LENGTH 1
#define DEVICEID_LENGTH 1
#define CRC_LENGTH 4
#define KEY_LENGTH 16

#define VERSION_OFFSITE 0
#define LENGTH_OFFSITE (VERSION_OFFSITE + VERSION_LENGTH)
#define ENCODEDEVICEID_OFFSITE (LENGTH_OFFSITE + DEVICEID_LENGTH)

// Key just for test
// Get it from tanx, format like: 
// f7dbeb735b7a07f1cfca79cc1dfe4fa4
static uchar g_key[] = {
    0x13, 0x76, 0x59, 0x77, 0xec, 0x84, 0x52, 0xf5,
    0xa3, 0xc6, 0x56, 0x87, 0xaf, 0x31, 0x58, 0x12
};

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

// Step list for decoding original deviceId
// If this function returns true, real deviceId will 
// return to realDeviceId variable
// If this function returns false, it means format error or checksum
// error, realDeviceId variable is invalid
static bool decodeDeviceId(uchar* src, uchar* realDeviceId)
{
    // Get version and check
    int v = *src;
    if (v != 1)
    {
        cerr << "version:" << v << "!=1, error" << endl;
        return false;
    }

    //get deviceId length
    uchar len = *(src + VERSION_LENGTH);

    // Copy key
    uchar buf[64];
    memcpy(buf, g_key, KEY_LENGTH);

    // ctxBuf=MD5(key)
    uchar ctxBuf[KEY_LENGTH];
    MD5(buf, KEY_LENGTH, ctxBuf);

    // Get deviceId
    uchar deviceId[64];
    uchar* pDeviceId = deviceId;
    uchar* pEncodeDeviceId = src + ENCODEDEVICEID_OFFSITE;
    uchar* pXor = ctxBuf;
    uchar* pXorB = pXor; 
    uchar* pXorE = pXorB + KEY_LENGTH; 
    for (int i=0; i<len; ++i)
    {
        if (pXor == pXorE)
        {
            pXor = pXorB;
        }
        *pDeviceId++ = *pEncodeDeviceId++ ^ *pXor++;
    }

    // Copy decode deviceId
    memmove(realDeviceId, deviceId, len);

    // Calc crc and compare with src
    // If not match, it is illegal

    // copy(version+length+deviceId+key)
    uchar* pbuf = buf;
    memcpy(pbuf, src, VERSION_LENGTH + DEVICEID_LENGTH);// copy version+length
    pbuf += VERSION_LENGTH + DEVICEID_LENGTH;

    // Notice: here is deviceId not realDeviceId
    // More important for big endian !
    memcpy(pbuf, deviceId, len);// copy deviceId
    pbuf += len;
    memcpy(pbuf, g_key, KEY_LENGTH);// copy key

    // MD5(version+length+deviceId+key)
    MD5(buf, VERSION_LENGTH + DEVICEID_LENGTH + len + KEY_LENGTH, ctxBuf);
    if (0 != memcmp(ctxBuf, src + ENCODEDEVICEID_OFFSITE + len, CRC_LENGTH))
    {
        cerr << "checksum error!" <<endl;
        return false;
    }

    return true;
}

/*
 * More sample string for encoded deviceId:
 *   1. src: AQ927DKCkp3HaJ+1n60VSBngmY2K
 *      result:
 *           Decode deviceId ok, deviceId = 493002407599521
 *   2. src: ASRy5TGF4Z7HbYXG4NISVxy1c+wsi5CWwHXqxuSmFj8QwHDiQ/CVPeSb
 *      result:
 *           Decode deviceId ok, deviceId = 0007C145-FFF2-4119-9293-BFB26E8D27BB
 *   3. src: AREDlCzw4IKwG4XE4rllPwW0c166xOg=
 *      result:
 *           Decode deviceId ok, deviceId = AA-BB-CC-DD-EE-01
 */
//int main(int argc, char* argv[])
//{
    // The sample string for encoded deviceId
    // result: 
    //    Decode deviceId ok, deviceId = 0007C145-FFF2-4119-9293-BFB26E8D27BB
//    char src[] = "ASRy5TGF4Z7HbYXG4NISVxy1c+wsi5CWwHXqxuSmFj8QwHDiQ/CVPeSb";

int decodeTanxDeviceId(char *src, uchar *deviceId)
{
    int len = strlen(src);
    int origLen = base64DecodedLength(len);
    uchar* orig = new uchar[origLen];

    // Standard BASE64 decoder using ssl library
    origLen = base64Decode(src, len, (char*)orig, origLen);

//    uchar deviceId[64] = {'\0'};
    if (decodeDeviceId(orig, deviceId))
    {
        cout << "Decode deviceId ok, deviceId = " << deviceId  << endl;
    }

    delete [] orig;
    return 0;
}
