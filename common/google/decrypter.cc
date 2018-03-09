// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Decrypter is sample code showing the steps to decrypt and verify 64-bit
// values. It uses the Base 64 decoder in the OpenSSL library.

#include <endian.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>
#include <string.h>
#include <sys/time.h>
//#include "../../realtime-bidding.pb.h"
#include "decode.h"

typedef int                 int32;
typedef long long           int64;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;
typedef unsigned char       uchar;

// Definition of ntohll
inline uint64 ntohll(uint64 host_int) {
#if defined(__LITTLE_ENDIAN)
  return static_cast<uint64>(ntohl(static_cast<uint32>(host_int >> 32))) |
      (static_cast<uint64>(ntohl(static_cast<uint32>(host_int))) << 32);
#elif defined(__BIG_ENDIAN)
  return host_int;
#else
#error Could not determine endianness.
#endif
}

namespace {
using std::string;

// The following sizes are all in bytes.
const int32 kInitializationVectorSize = 16;
const int32 kCiphertextSize = 8;
const int32 kSignatureSize = 4;
const int32 kEncryptedValueSize =
    kInitializationVectorSize + kCiphertextSize + kSignatureSize;
const int32 kKeySize = 32;  // size of SHA-1 HMAC keys.
const int32 kHashOutputSize = 20;  // size of SHA-1 hash output.
const int kBlockSize = 20;  // This is a block cipher with fixed block size.

// Retrieves the timestamp embedded in the initialization vector.
void GetTime(const char* initialization_vector, struct timeval* tv) {
  uint32 val;
  memcpy(&val, initialization_vector, sizeof(val));
  tv->tv_sec = htonl(val);
  memcpy(&val, initialization_vector + sizeof(val), sizeof(val));
  tv->tv_usec = htonl(val);
}

// Takes an unpadded base64 string and adds padding.
string AddPadding(const string& b64_string) {
  if (b64_string.size() % 4 == 3) {
    return b64_string + "=";
  } else if (b64_string.size() % 4 == 2) {
    return b64_string + "==";
  }
  return b64_string;
}

// Adapted from http://www.openssl.org/docs/crypto/BIO_f_base64.html
// Takes a web safe base64 encoded string (RFC 3548) and decodes it.
// Normally, web safe base64 strings have padding '=' replaced with '.',
// but we will not pad the ciphertext. We add padding here because
// openssl has trouble with unpadded strings.
string B64Decode(const string& encoded) {
  string padded = AddPadding(encoded);
  // convert from web safe -> normal base64.
  int32 index = -1;
  while ((index = padded.find_first_of('-', index + 1)) != string::npos) {
    padded[index] = '+';
  }
  index = -1;
  while ((index = padded.find_first_of('_', index + 1)) != string::npos) {
    padded[index] = '/';
  }

  // base64 decode using openssl library.
  const int32 kOutputBufferSize = 256;
  char output[kOutputBufferSize];

  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO* bio = BIO_new_mem_buf(const_cast<char*>(padded.data()),
                             padded.length());
  bio = BIO_push(b64, bio);
  int32 out_length = BIO_read(bio, output, kOutputBufferSize);
  BIO_free_all(bio);
  return string(output, out_length);
}

inline char* string_as_array(string* str) {
  return str->empty() ? NULL : &*str->begin();
}

inline const char* string_as_array(const string& str) {
  return string_as_array(const_cast<string*>(&str));
}

// This method decrypts the ciphertext using the encryption key and verifies
// the integrity bits with the integrity key. The encrypted format is:
//   {initialization_vector (16 bytes)}{ciphertext}{integrity (4 bytes)}
// https://developers.google.com/ad-exchange/rtb/response-guide/decrypt-hyperlocal,
// https://developers.google.com/ad-exchange/rtb/response-guide/decrypt-price
// and https://support.google.com/adxbuyer/answer/3221407?hl=en have more
// details about the encrypted format of hyperlocal, winning price,
// IDFA, hashed IDFA and Android Advertiser ID.
//
// If DecryptByteArray returns true, cleartext contains the value encrypted in
// ciphertext.
// If DecryptByteArray returns false, the ciphertext could not be decrypted
// (e.g. the signature did not match).
bool DecryptByteArray(
    const string& ciphertext, const string& encryption_key,
    const string& integrity_key, string* cleartext) {
  // Step 1. find the length of initialization vector and clear text.
  const int cleartext_length =
     ciphertext.size() - kInitializationVectorSize - kSignatureSize;
  if (cleartext_length < 0) {
    // The length can't be correct.
    return false;
  }

  string iv(ciphertext, 0, kInitializationVectorSize);

  // Step 2. recover clear text
  cleartext->resize(cleartext_length, '\0');
  const char* ciphertext_begin = string_as_array(ciphertext) + iv.size();
  const char* const ciphertext_end = ciphertext_begin + cleartext->size();
  string::iterator cleartext_begin = cleartext->begin();

  bool add_iv_counter_byte = true;
  while (ciphertext_begin < ciphertext_end) {
    uint32 pad_size = kHashOutputSize;
    uchar encryption_pad[kHashOutputSize];

    if (!HMAC(EVP_sha1(), string_as_array(encryption_key),
              encryption_key.length(), (uchar*)string_as_array(iv),
              iv.size(), encryption_pad, &pad_size)) {
      printf("Error: encryption HMAC failed.\n");
      return false;
    }

    for (int i = 0;
         i < kBlockSize && ciphertext_begin < ciphertext_end;
         ++i, ++cleartext_begin, ++ciphertext_begin) {
      *cleartext_begin = *ciphertext_begin ^ encryption_pad[i];
    }

    if (!add_iv_counter_byte) {
      char& last_byte = *iv.rbegin();
      ++last_byte;
      if (last_byte == '\0') {
        add_iv_counter_byte = true;
      }
    }

    if (add_iv_counter_byte) {
      add_iv_counter_byte = false;
      iv.push_back('\0');
    }
  }

  // Step 3. Compute integrity hash. The input to the HMAC is cleartext
  // followed by initialization vector, which is stored in the 1st section of
  // ciphertext.
  string input_message(kInitializationVectorSize + cleartext->size(), '\0');
  memcpy(string_as_array(&input_message),
         string_as_array(cleartext), cleartext->size());
  memcpy(string_as_array(&input_message) + cleartext->size(),
         string_as_array(ciphertext), kInitializationVectorSize);

  uint32 integrity_hash_size = kHashOutputSize;
  unsigned char integrity_hash[kHashOutputSize];
  if (!HMAC(EVP_sha1(), string_as_array(integrity_key),
            integrity_key.length(), (uchar*)string_as_array(input_message),
            input_message.size(), integrity_hash, &integrity_hash_size)) {
    printf("Error: integrity HMAC failed.\n");
    return false;
  }

  return memcmp(ciphertext_end, integrity_hash, kSignatureSize) == 0;
}

// This function is to decrypt winning price.
// Note that decrypting IDFA (or Android Advertiser Id, or hashed IDFA) is
// very similar to decrypting winning price, except that
//  1. IDFA has 16 bytes instead of 8 bytes;
//  2. IDFA is a byte array, therefore doesn't need to switch byte order to
//     handle big endian vs. little endian issue.
bool DecryptWinningPrice(
    const string& encrypted_value, const string& encryption_key,
    const string& integrity_key, int64* value) {
  string cleartext;
  if (!DecryptByteArray(encrypted_value, encryption_key, integrity_key,
                        &cleartext)) {
    // fail to decrypt
    return false;
  }

  if (cleartext.size() != sizeof(value)) {
    // cleartext has wrong size
    return false;
  }

  // Switch to host byte order.
  *value = ntohll(*reinterpret_cast<const int64*>(string_as_array(cleartext)));
  return true;
}

}  // namespace

// An example program that decodes the hardcoded ciphertext using hardcoded
// keys. First it base64 decodes the encrypted value, then it calls Decrypt to
// decrypt the ciphertext and verify its integrity.

int DecodeByteArray(const string &encrypttext, string &cleartext)
{
	const char kEncKey[] = {
		0x42, 0xe6, 0x05, 0x29, 0x9a, 
		0x42, 0x23, 0x64, 0x96, 0x60, 
		0x90, 0xe9, 0x7e, 0xcd, 0x08, 
		0xb1, 0xff, 0xef, 0x29, 0x3c, 
		0xc0, 0x6a, 0xd4, 0xad, 0x30, 
		0x5a, 0x5c, 0x8d, 0x14, 0x05, 
		0xb3, 0x0d
	};
	const char kIntKey[] = {
		0xc9, 0xca, 0x02, 0x35, 0x19, 
		0x3a, 0x7a, 0x23, 0x96, 0xad, 
		0x21, 0x62, 0x65, 0xd6, 0x08, 
		0x6c, 0xbf, 0xac, 0xbb, 0xaf, 
		0xe7, 0x20, 0x6e, 0x85, 0x3b, 
		0x93, 0xb6, 0x13, 0x07, 0xf6, 
		0x73, 0x37
	};
	const string encryption_key(kEncKey, kKeySize);
	const string integrity_key(kIntKey, kKeySize);
	const string encrypted_value(encrypttext, encrypttext.size());

	bool success = DecryptByteArray(
		encrypttext, encryption_key, integrity_key, &cleartext);

	if (!success) {
		printf("Failed to decrypt byte array.\n");
		return 1;
	}

//	printf("Decryption succeeded.\n");
	return 0;
}

int DecodeWinningPrice(char *encodedprice, double *value) {
	const char kEncryptionKey[] = {
		0x42, 0xe6, 0x05, 0x29, 0x9a, 
		0x42, 0x23, 0x64, 0x96, 0x60, 
		0x90, 0xe9, 0x7e, 0xcd, 0x08, 
		0xb1, 0xff, 0xef, 0x29, 0x3c, 
		0xc0, 0x6a, 0xd4, 0xad, 0x30, 
		0x5a, 0x5c, 0x8d, 0x14, 0x05, 
		0xb3, 0x0d
	};
	const char kIntegrityKey[] = {
		0xc9, 0xca, 0x02, 0x35, 0x19, 
		0x3a, 0x7a, 0x23, 0x96, 0xad, 
		0x21, 0x62, 0x65, 0xd6, 0x08, 
		0x6c, 0xbf, 0xac, 0xbb, 0xaf, 
		0xe7, 0x20, 0x6e, 0x85, 0x3b, 
		0x93, 0xb6, 0x13, 0x07, 0xf6, 
		0x73, 0x37
	};
	string encryption_key(kEncryptionKey, kKeySize);
	string integrity_key(kIntegrityKey, kKeySize);

	// This is an example of the encrypted data. It has a fixed length of 38
	// characters. The two padding characters are removed. It decodes to a string
	// of 28 bytes. The decrypted value should be 709959680.
	const string kB64EncodedValue(encodedprice);
	string encrypted_value = B64Decode(kB64EncodedValue);
	if (encrypted_value.size() != kEncryptedValueSize) {
		printf("Error: unexpected ciphertext length: %lu\n",
			encrypted_value.size());
		return 1;
	}

	int64 va = 0;
	bool success = DecryptWinningPrice(
		encrypted_value, encryption_key, integrity_key, &va);
	if (success) {
		*value = (double)va;
		/*printf("The value is:   %lld\n", value);
		struct timeval tv;
		GetTime(encrypted_value.data(), &tv);
		struct tm tm;
		localtime_r(&tv.tv_sec, &tm);
		printf("Sent on %04d-%02d-%02d|%02d:%02d:%02d.%06ld\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);
			*/
	} else {
		printf("Failed to decrypt value.\n");
	}
	return 0;
}

bool Hex2Char(char const* szHex, unsigned char& rch)
{
	rch = 0;
	unsigned char lch;

	for(int i=0; i<2; i++)
	{
		lch = (unsigned char)(*(szHex + i));
		if(lch >='0' && lch <= '9')
			rch = (rch << 4) + (lch - '0');
		else if(lch >='A' && lch <= 'F')
			rch = (rch << 4) + (lch - 'A' + 10);
		else if(lch >='a' && lch <= 'f')
			rch = (rch << 4) + (lch - 'a' + 10);
		else 
			return false;
	}
	return true;
}  

string Hex2String(string hex)
{
	int i;
	unsigned char ch;
	string dst;

	for(i=0; i < hex.size()/2; ++i)
	{
		if (Hex2Char(hex.c_str() + 2*i, ch)) {
			dst.append(1, ch);
		}
	}    
	return dst;
}

/*int main(int argc, char** argv) {
 char kB64EncodedValue[] = "SjpvRwAB4kB7jEpgW5IA8p73ew9ic6VZpFsPnA";
 double price = 0;
  int status = DecodeWinningPrice(kB64EncodedValue, &price);
  if (status != 0) {
    return status;
  }
  else
  {
	  printf("price :%lf\n", price);
  }
  const string hyperlocal("E2014EA201246E6F6E636520736F7572636501414243C0ADF6B9B6AC17DA218FB50331EDB376701309CAAA01246E6F6E636520736F7572636501414243C09ED4ECF2DB7143A9341FDEFD125D96844E25C3C202466E6F6E636520736F7572636502414243517C16BAFADCFAB841DE3A8C617B2F20A1FB7F9EA3A3600256D68151C093C793B0116DB3D0B8BE9709304134EC9235A026844F276797");
  string encrypttext = Hex2String(hyperlocal);
  BidRequest bid_req;
  BidRequest_HyperlocalSet hyperlocalsetinner;
  bool parsesuccess = bid_req.ParsePartialFromString(encrypttext);
  if(parsesuccess)
  {
	  printf("parse success\n");
  }

  if(bid_req.has_encrypted_hyperlocal_set())
  {
	  string cleartext;
	  printf("has_encrypted_hyperlocal_set\n");
	  const BidRequest_HyperlocalSet&  hyperlocalset =bid_req.hyperlocal_set();
	  // This field currently contains at most one hyperlocal polygon.
	 const string encrypted_hyperlocal_set = bid_req.encrypted_hyperlocal_set();
	  int success = DecodeByteArray(encrypted_hyperlocal_set, cleartext);

	  if (success != 0) {
		  printf("Failed to decrypt byte array.\n");
		  return 1;
	  }
	  parsesuccess = false;

	  parsesuccess = hyperlocalsetinner.ParsePartialFromString(cleartext);
	  if(parsesuccess)
	  {
		  printf("parse cleartext success\n");
		  //const BidRequest_HyperlocalSet&  hyperlocalset1 =bid_req1.hyperlocal_set();
		  if(hyperlocalsetinner.hyperlocal().size() > 0)
		  {
			  printf("hyperlocal.size >0\n");
			  const BidRequest_Hyperlocal& hyperlocal = hyperlocalsetinner.hyperlocal(0);
			  // The mobile device can be at any point inside the geofence polygon defined
			  // by a list of corners. Currently, the polygon is always a parallelogram
			  // with 4 corners.
			  if(hyperlocal.corners_size() >0)
			  {
				  const BidRequest_Hyperlocal_Point& point = hyperlocal.corners(0);
				  printf("lat :%lf and lon :%lf\n",point.latitude(), point.longitude());
			  }
		  }
	  }
  }
  return 0;
}*/