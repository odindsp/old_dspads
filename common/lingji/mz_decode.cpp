#include <iostream>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "openssl_aes.h"
#include "base64.h"
#include "mz_decode.h"

#define EMPTYSTR ""

static bool hexChar2int(const unsigned char & c, unsigned char & i){
    if(c >= '0' && c <= '9'){
        i = c - '0';
        return true;
    }else if(c >= 'a' && c <= 'f'){
        i = c - 'a' + 10;
        return true;
    }else if(c >= 'A' && c <= 'F'){
        i = c - 'A' + 10;
        return true;
    }

    return false;

}

static bool hex2array(const unsigned char * hex, const uint32_t & len, unsigned char * result){
    if(32 != len){
        return false;
    }
    static unsigned char tmp_a, tmp_b;
    for(uint32_t i = 0; i < len; i += 2){
        if(!hexChar2int(hex[i], tmp_a) || 
            !hexChar2int(hex[i + 1], tmp_b)){
            return false;
        }
        result[i/2] = (tmp_a << 4) + tmp_b;
    }

    return true;

}

std::string mz_encode(const char * hexkey, const char * plaintext){
    unsigned char key[16];
    if(! hex2array((const unsigned char *) hexkey, strlen(hexkey), key)){
        std::cerr << "hex2array(...) fail." << std::endl;
        return EMPTYSTR;
    }

    int len_plaintext = strlen(plaintext);

    //aes encode
    unsigned char * aes_encoded_text = NULL;
    aes_encoded_text = aes_128_ecb_encrypt(plaintext, &len_plaintext, (const char *)key);
    if(NULL == aes_encoded_text){
        return EMPTYSTR;
    }
    //base64 encode again
    std::string base64_aes_encoded_text;
    base64_aes_encoded_text = base64_encode(aes_encoded_text, len_plaintext);

    free(aes_encoded_text);

    return base64_aes_encoded_text;

}

std::string mz_decode(const char * hexkey, const char * encodedtext){
    unsigned char key[16];
    if(! hex2array((const unsigned char *) hexkey, strlen(hexkey), key)){
        std::cerr << "hex2array(...) fail." << std::endl;
        return EMPTYSTR;
    }

    //base64 decode
    std::string base64_decoded_text = EMPTYSTR;
    int base64_decode_text_len = 0;
    base64_decoded_text = base64_decode(encodedtext);
    base64_decode_text_len = base64_decoded_text.length();

    //aes decode again
    char* aes_base64_decoded_text = NULL;
    aes_base64_decoded_text = aes_128_ecb_decrypt((const unsigned char*)base64_decoded_text.c_str(), &base64_decode_text_len, (const char *)key);
    if(NULL == aes_base64_decoded_text){
        return EMPTYSTR;
    }
    std::string plain = EMPTYSTR;
    plain.assign(aes_base64_decoded_text, base64_decode_text_len);

    free(aes_base64_decoded_text);

    return plain;

}

int DecodeWinningPrice(char *encodedprice, double *value) {
	const char* hexkey = "e3e65f28ccd641199e09184526e8d417";
	std::string decodeprice = mz_decode(hexkey, encodedprice);
	std::string price;
	//price string is like as "100.0_123123123" -- price_timestamp - millisecond
	if (!decodeprice.empty()){
		int pos = decodeprice.find("_");
		if (pos != std::string::npos){
			price = decodeprice.substr(0, pos);
			if (!price.empty()) {
				*value = atof(price.c_str());
				return 0;
			}
		}
	}
	return -1;
}

