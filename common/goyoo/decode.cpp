#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
//#include <machine/endian.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <string>

#include "decode.h"
using namespace std;
using std::string;

static const int INITIALIZATION_VECTOR_SIZE = 16;
static const int CIPHER_TEXT_SIZE = 8;
static const int SIGNATURE_SIZE = 4;
static const int ENCRYPTED_VALUE_SIZE =
        INITIALIZATION_VECTOR_SIZE + CIPHER_TEXT_SIZE + SIGNATURE_SIZE;
static const int HASH_OUTPUT_SIZE = 20;
const char integrity_key_v[]  = {
	0x0c, 0xf6, 0x92, 0x07, 0x66, 0xbc, 0xf2, 0xa7, 0xa5, 0x1b, 0x92, 0xc8, 0x13, 0xb8, 0x8a, 0x8e, 0x80, 0x65, 0xe4, 0xd1, 0x41, 0x7c, 0x17, 0xc7, 0x9e, 0x87, 0x85, 0x9b, 0x55, 0x62, 0xba, 0x92
};

const char encryption_key_v[] = {
	0xf6, 0xb0, 0x72, 0xdc, 0x60, 0xb5, 0xa3, 0x71, 0xce, 0xdc, 0x13, 0x48, 0xdf, 0x52, 0xa8, 0x2a, 0x7d, 0x20, 0x5d, 0xf1, 0x22, 0x36, 0xfa, 0xc5, 0x70, 0x15, 0x84, 0xe6, 0xf1, 0x3f, 0x8d, 0xe0
};

static const string ENCRYPTION_KEY(encryption_key_v, 32);
static const string INTEGRITY_KEY(integrity_key_v, 32);

// Definition of htonll
inline uint64_t htonll(uint64_t net_int) {
#if defined(__LITTLE_ENDIAN)
    return static_cast<uint64_t>(htonl(static_cast<uint32_t>(net_int >> 32))) |
            (static_cast<uint64_t>(htonl(static_cast<uint32_t>(net_int))) << 32);
#elif defined(__BIG_ENDIAN)
    return net_int;
#else
#error Could not determine endianness.
#endif
}
// Definition of ntohll
inline uint64_t ntohll(uint64_t host_int) {
#if defined(__LITTLE_ENDIAN)
    return static_cast<uint64_t>(ntohl(static_cast<uint32_t>(host_int >> 32))) |
            (static_cast<uint64_t>(ntohl(static_cast<uint32_t>(host_int))) << 32);
#elif defined(__BIG_ENDIAN)
    return host_int;
#else
#error Could not determine endianness.
#endif
}

// The actual decryption method:
// Decrypts the ciphertext using the encryption key and verifies the integrity
// bits with the integrity key. The encrypted format is:
// {initialization_vector (16 bytes)}{ciphertext (8 bytes)}{integrity (4 bytes)}
// The value is encrypted as
// <value xor HMAC(encryption_key, initialization_vector)> so
// decryption calculates HMAC(encryption_key, initialization_vector) and xor's
// with the ciphertext to reverse the encryption. The integrity stage takes 4
// bytes of <HMAC(integrity_key, value||initialization_vector)> where || is
// concatenation.
// If Decrypt returns true, value contains the value encrypted in ciphertext.
// If Decrypt returns false, the value could not be decrypted (the signature
// did not match).
//
bool decrypt_int64(
    const std::string& encrypted_value, const std::string& encryption_key,
    const std::string& integrity_key, int64_t* value)
{
    // Compute plaintext.
    const uint8_t* initialization_vector = (uint8_t*)encrypted_value.data();
    //len(ciphertext_bytes) = 8 bytes
    const uint8_t* ciphertext_bytes =
            initialization_vector + INITIALIZATION_VECTOR_SIZE;
    //signatrue = initialization_vector + INITIALIZATION_VECTOR_SIZE(16) + CIPHER_TEXT_SIZE(8)
    //len(signature) = 4 bytes
     //len(signature) = 4 bytes
    const uint8_t* signature = ciphertext_bytes + CIPHER_TEXT_SIZE;

    uint32_t pad_size = HASH_OUTPUT_SIZE;
    uint8_t price_pad[HASH_OUTPUT_SIZE];

    //get price_pad using openssl/hmac.h
    if (!HMAC(EVP_sha1(), encryption_key.data(), encryption_key.length(),
              initialization_vector, INITIALIZATION_VECTOR_SIZE, price_pad,
              &pad_size)) {
        return false;
    }
    uint8_t plaintext_bytes[CIPHER_TEXT_SIZE];
    for (int32_t i = 0; i < CIPHER_TEXT_SIZE; ++i) {
        plaintext_bytes[i] = price_pad[i] ^ ciphertext_bytes[i];
    }
    //print debug_info
   /* printf("\nciphertext_bytes:");
    for(int size=0;size<CIPHER_TEXT_SIZE;size++){
        printf("%02x",ciphertext_bytes[size]);
    }
    printf("\nprice_pad:");
    for(int size=0;size<HASH_OUTPUT_SIZE;size++){
        printf("%02x",price_pad[size]);
}*/

    //value = ntohllprice_pad ^ ciphertext_bytes) 
    memcpy(value, plaintext_bytes, CIPHER_TEXT_SIZE);
    *value = ntohll(*value);  // Switch to host byte order.
//    cout << endl << "value:" << *value ;

    // Verify integrity bits.
    uint32_t integrity_hash_size = HASH_OUTPUT_SIZE;
    uint8_t integrity_hash[HASH_OUTPUT_SIZE];
    const int32_t INPUT_MESSAGE_SIZE = CIPHER_TEXT_SIZE + INITIALIZATION_VECTOR_SIZE;
    uint8_t input_message[INPUT_MESSAGE_SIZE];

    memcpy(input_message, plaintext_bytes, CIPHER_TEXT_SIZE);
    memcpy(input_message + CIPHER_TEXT_SIZE,
           initialization_vector,
           INITIALIZATION_VECTOR_SIZE);

    if (!HMAC(EVP_sha1(), integrity_key.data(), integrity_key.length(),
              input_message, INPUT_MESSAGE_SIZE, integrity_hash,
              &integrity_hash_size)) {
        return false;
    }

    //print debug_info
  /* printf("\ninput_message:");
    for(int32_t size=0;size<INPUT_MESSAGE_SIZE;size++){
        printf("%02x",input_message[size]);
    }
    printf("\ninitializatio_hash:");
    for(uint32_t size=0;size<integrity_hash_size;size++){
        printf("%02x",integrity_hash[size]);
    }
    printf("\nsignatre:");
    for(int32_t size=0;size<SIGNATURE_SIZE;size++){
        printf("%02x",signature[size]);
    }
    printf("\n");*/
    return memcmp(integrity_hash, signature, SIGNATURE_SIZE) == 0;
}

// Adapted from http://www.openssl.org/docs/crypto/BIO_f_base64.html
bool base64_decode(const std::string& encoded, string *output)
{
    // Alwarys assume that the length of base64 encoded string
    // is larger than decoded one
    char *temp = static_cast<char*>(malloc(sizeof(char) * encoded.length()));
    if (temp == NULL) {
        return false;
    }
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bio = BIO_new_mem_buf(const_cast<char*>(encoded.data()),
                               encoded.length());
    bio = BIO_push(b64, bio);
    int32_t out_length = BIO_read(bio, temp, encoded.length());
    BIO_free_all(bio);
    output->assign(temp, out_length);
    free(temp);
    return true;
}


bool web_safe_base64_decode(const std::string& encoded, string * output)
{
    // convert from web safe -> normal base64.
    string normal_encoded = encoded;
    size_t index = string::npos;
    while ((index = normal_encoded.find_first_of('-', index + 1)) != string::npos) {
        normal_encoded[index] = '+';
    }
    index = string::npos;
    while ((index = normal_encoded.find_first_of('_', index + 1)) != string::npos) {
        normal_encoded[index] = '/';
    }
    return base64_decode(normal_encoded, output);
}

//add padding 
std::string add_padding(const std::string& b64_string) {
    if (b64_string.size() % 4 == 3) {
        return b64_string + "=";
    } else if (b64_string.size() % 4 == 2) {
        return b64_string + "==";
    }
    return b64_string;
}


/*
密文示例和真实价格
    aiA6VefRdkYNuCxAlZ_Df6fb9Xbf49dRtgo6Bw 500
    2aRpExhGduTfI2BwaY8aEh1eWiZXiNcXevEe1A 88
    RZD7LSw2Qlkg2NzdiU9KmtVgQrbVVZX7_Ehf3w 42
    RtxR_sBr8AwOjAxD0p9omwlXRjKEjtVd-YG2kA 65
    htkCCbDwfqcHR84scK3u9keskTs81lhA2B_-Mw 10000
*/

int DecodeWinningPrice(char *sourceStr, double *price)
{
	std::string encryption_value = string(sourceStr);
	std::string padded = add_padding(encryption_value);
	std::string decode_value;

	int64_t act_value = 0;
	web_safe_base64_decode(padded,&decode_value);
	
	if (!decrypt_int64(decode_value,ENCRYPTION_KEY,INTEGRITY_KEY,&act_value))
	{
		cout<<"decryption failed!!"<<endl;
		*price = 0;
		return 1;
	}
	
	*price = act_value;

	return 0;
}

/*
int main(int argc, const char *argv[])
{
	char *encryption_value = "RZD7LSw2Qlkg2NzdiU9KmtVgQrbVVZX7_Ehf3w";
	
	double price = 0.0;
		
	DecodeWinningPrice(encryption_value, &price);
	
	cout<<"price:"<<price<<endl;
}
*/