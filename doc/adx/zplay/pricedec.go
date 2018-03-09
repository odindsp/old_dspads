package main

import (
	"bytes"
	"crypto/hmac"
	"crypto/sha1"
	"encoding/base64"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"strings"
)

func main() {
	fmt.Println(Decode("fa4cb7dce5784c369caf61fa93e3185a", "1ce14bcd923246caa14e3fb9e06f9d79", "s6xCVl2H6xxY26wF3mA-HA"))
}

func Decode(dec_key, sig_key, price_enc string) (float64, error) {
	data, err := base64url_decode(price_enc)
	if err != nil {
		return 0, err
	}

	if len(data) != 16 {
		return 0, errors.New("Illegal base64 string")
	}

	time_ms_stamp_bytes := data[:4]
	enc_price := data[4:12]
	sig := data[12:16]

	dec_key_bytes := hex2bin(dec_key)

	pad := sha1_hmac(time_ms_stamp_bytes, dec_key_bytes)
	dec_price := xor_bytes(pad, enc_price, 8)

	var price float64

	dec_price_buf := bytes.NewBuffer(dec_price)
	binary.Read(dec_price_buf, binary.LittleEndian, &price)

	//校验
	sig_key_bytes := hex2bin(sig_key)
	osig := sha1_hmac(append(dec_price, time_ms_stamp_bytes...), sig_key_bytes)
	if bytes.Compare(sig, osig[:len(sig)]) == 0 {
		//校验成功
		return price, nil
	}

	return price, errors.New("signature is illegal")
}

func xor_bytes(b1, b2 []byte, length int) []byte {
	new_b := make([]byte, length)
	for i := 0; i < length; i++ {
		new_b[i] = b1[i] ^ b2[i]
	}
	return new_b
}

func hex2bin(s string) []byte {
	ret, _ := hex.DecodeString(s)
	return ret
}

func sha1_hmac(data, key []byte) []byte {
	mac := hmac.New(sha1.New, key)
	mac.Write(data)
	return mac.Sum(nil)
}

func base64url_encode(data []byte) string {
	ret := base64.StdEncoding.EncodeToString(data)
	return strings.Map(func(r rune) rune {
		switch r {
		case '+':
			return '-'
		case '/':
			return '_'
		}

		return r
	}, ret)
}

func base64url_decode(s string) ([]byte, error) {
	base64Str := strings.Map(func(r rune) rune {
		switch r {
		case '-':
			return '+'
		case '_':
			return '/'
		}

		return r
	}, s)

	if pad := len(base64Str) % 4; pad > 0 {
		base64Str += strings.Repeat("=", 4-pad)
	}

	return base64.StdEncoding.DecodeString(base64Str)
}
