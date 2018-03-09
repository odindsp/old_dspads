/* Baidu decode */
#ifndef DECODE_H_
#define DECODE_H_
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "../base64.h"
#include "../algo_aes.h"

extern "C"
{
int DecodeWinningPrice(char *encodedprice, double *value);
}
#endif //DECODE_H_
