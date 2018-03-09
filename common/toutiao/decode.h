#ifndef _DECODE_H_
#define _DECODE_H_

#include "../type.h"

int decodeTanxDeviceId(char *src, uchar *deviceId);
extern "C" int DecodeWinningPrice(char *src, double *price);
#endif /* DECODE_H_ */
