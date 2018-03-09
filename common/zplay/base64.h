/* 
 * File:   base64.h
 * Author: Administrator
 *
 * Created on 2015年12月2日, 下午4:02
 */

#ifndef BASE64_H
#define BASE64_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
    @description:
        Converts n bytes pointed to by src into base-64 representation.
*/
extern char *encode_base64(const void *src, size_t n);

/*
    @description:
        Converts a valid base-64 string into a decoded array of bytes
        and stores the number of decoded bytes in the object pointed
        to by n.
*/
extern void *decode_base64(const char *src, size_t *n);

/*
    @description:
        Locates the index value of a base-64 character for decoding.
*/
static size_t digit_value(char ch);

/*
    @description:
        Appends a newline to dst if the value of end is at the correct
        position. Returns the updated value of end, which may not change
        if a newline was not appended.
*/
static void check_append_newline(char *dst, size_t *end);

/*
    @description:
        Converts a 24-bit block represented by 3 octets into four base-64
        characters and stores them in dst starting at end. Returns the
        updated value of end for convenience.
*/
static void convert_block(char *dst, size_t *end, const uint8_t *src, size_t bytes);

extern void *base64url_decode(const char *src);

#endif /* BASE64_H */

