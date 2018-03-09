/*
   base64.h
   Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
   */
#ifndef BASE64_H
#define BASE64_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

	extern char* base64_encode(const char *data);
	extern char* base64_decode(const char *data);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_H */
