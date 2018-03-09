#ifndef _SSP_BASE64_H_
#define _SSP_BASE64_H_
#include "ngx_task_define.h"
#include <iostream>
using namespace std;


#define NGX_ESCAPE_URI            0
#define NGX_ESCAPE_ARGS           1
#define NGX_ESCAPE_URI_COMPONENT  2
#define NGX_ESCAPE_HTML           3
#define NGX_ESCAPE_REFRESH        4
#define NGX_ESCAPE_MEMCACHED      5
#define NGX_ESCAPE_MAIL_AUTH      6

#define NGX_UNESCAPE_URI       1
#define NGX_UNESCAPE_REDIRECT  2

#define ngx_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define ngx_base64_decoded_length(len)  (((len + 3) / 4) * 3)

typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef intptr_t        ngx_flag_t;


void
ngx_encode_base64(ngx_str_t *dst, ngx_str_t *src);

ngx_int_t
ngx_decode_base64(ngx_str_t *dst, ngx_str_t *src);

ngx_int_t
ngx_decode_base64url(ngx_str_t *dst, ngx_str_t *src);

string base64_decode(std::string const& encoded_string);

void
ngx_unescape_uri(u_char **dst, u_char **src, size_t size, ngx_uint_t type);

#endif
