#ifndef _NGX_TASK_DEFINE_H
#define _NGX_TASK_DEFINE_H
#include "linux.h"

typedef n64 ngx_int_t;
typedef unsigned char u_char;
typedef uintptr_t        ngx_uint_t;

typedef struct
{
	ngx_int_t len;
	u_char* data;
}ngx_str_t;
#endif

