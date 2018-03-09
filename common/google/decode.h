#ifndef DECODE_H__
#define DECODE_H_

#include "../type.h"

int DecodeByteArray(const string &encrypttext, string &cleartext);
extern "C"
{
int DecodeWinningPrice(char *encodedprice, double *value);
//int DecodeByteArray(const string &encrypttext, string &cleartext);
}
#endif /* DECODE_H_ */
