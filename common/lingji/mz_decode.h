#ifndef MZ_DECODE_H
#define MZ_DECODE_H
#include <string>

//std::string mz_encode(const char * hexkey, const char * plaintext);
//std::string mz_decode(const char * hexkey, const char * encodedtext);

extern "C"
{
int DecodeWinningPrice(char *encodedprice, double *value);
}

#endif //~MZ_DECODE_H
