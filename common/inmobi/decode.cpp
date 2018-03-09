/* inmobi decode */
#include <stdlib.h>
#include <stdio.h>

extern "C"
int DecodeWinningPrice(char *encodedprice, double *value)
{
    *value = atof(encodedprice);
    return 0;
}
