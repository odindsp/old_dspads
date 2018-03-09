#ifndef __LINUX_H_INCLUDED__
#define __LINUX_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>             /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <glob.h>
#include <sys/vfs.h>            /* statfs() */

#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>        /* TCP_NODELAY, TCP_CORK */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include <time.h>               /* tzset() */
#include <malloc.h>             /* memalign() */
#include <limits.h>             /* IOV_MAX */
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <crypt.h>
#include <sys/utsname.h>        /* uname() */
#include <dlfcn.h>
#ifdef __cplusplus
}
#endif

#define _SYS_LINUX

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long int	u64;

typedef char				n8;
typedef short				n16;
typedef int					n32;
typedef long int			n64;

typedef n64		int64;
typedef u8		u_char;
//typedef u32		size_t;

#ifdef _SYS_LINUX
typedef n32					BOOL;
//typedef unsigned int 		DWORD;
typedef n32					HRESULT;

#define TRUE 1
#define FALSE 0
#define S_FALSE 1
#define S_OK	0
#define NGX_OK	   0
#define NGX_ERROR -1

#ifndef _UNICODE
#define TCHAR char
#else
#define TCHAR wchar_t
#endif

#define _STRING_NEXT_LINE	"\n"
#define _PATH_SPLIT			"/"
#define MAX_PATH			256

#define atoi64(x)(atoll(x))
#define ZeroMemory(x,y)(memset(x,0,y))
#define strcmpi(x,y)(strcasecmp(x,y))

#define _T(x)      x

//#define BAD_CAST
#define _THREAD_HEAD
#else
#include <windows.h>
#define _THREAD_HEAD		WINAPI
#define _STRING_NEXT_LINE	"\r\n"
#define _PATH_SPLIT			"\\"

#define atoi64(x)(_atoi64(x))
#endif

#endif
