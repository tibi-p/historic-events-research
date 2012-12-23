#ifndef UTIL_H_
#define UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

#define fseek64 _fseeki64
#define ftell64 _ftelli64
#define strtoull _strtoui64

#else

#define fseek64 fseeko64
#define ftell64 ftello64

#endif

char * concatenate(const char *left, const char *right);
long get_file_size(FILE *f);
long long get_file_size64(FILE *f);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_H_ */
