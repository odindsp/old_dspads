/* 
 * File:   price.h
 * Author: weihaibin
 *
 * Created on 2015年12月2日, 下午4:10
 */

#ifndef PRICE_H
#define PRICE_H

#include <stdio.h>

extern char* xor_bytes(char* b1,char* b2,size_t len);

extern double price_decode(const char* dec_key,const char* sign_key,const char* src);

int DecodeWinningPrice(char *source, double *price);

#endif /* PRICE_H */

