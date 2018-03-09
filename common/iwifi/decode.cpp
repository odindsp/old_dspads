/*iwifi decode*/
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "decode.h"
using namespace std;

extern "C"

double DecodePrice(long price, int user_secretkey)
{
        int base_secretkey = 32;

        return double((price >> base_secretkey - user_secretkey)) / 10000;
}

long encodePrice(double price, int user_secretkey)
{
        int base_secretkey = 32;
        long w = (long)(price * 10000);

        return ((w << base_secretkey) + user_secretkey);
}

int DecodeWinningPrice(char *encodedprice, double *value)
{
    long lprice = 0;
    double price= 0;
    int user_secretkey = 1024;

    lprice = atol(encodedprice);
    price = DecodePrice(lprice, user_secretkey);
    
  //  cout<<"price = "<<price<<endl;
    *value = price;
 
    return 0;
}
/*
int main()
{
    double price = 10.9999;
    double mprice = 0;
    int user_secretkey = 1024;
    long lprice = 0;
    double nprice = 0;
    lprice = encodePrice(price, user_secretkey);
    cout<<"price="<<price<<endl;    
    cout<<"encode price="<<lprice<<endl;

    mprice = DecodePrice(lprice, user_secretkey);

    cout<<"decode price="<<mprice<<endl;

    DecodeWinningPrice("472442107593728", &nprice);

    cout<<"nprice = "<<nprice<<endl;
}
*/
