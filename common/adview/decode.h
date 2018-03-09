#ifndef DECODE_H__
#define DECODE_H_

#include "../type.h"

extern "C"
{
int DecodeWinningPrice(char *encodedprice, double *value);
}
#endif /* DECODE_H_ */
