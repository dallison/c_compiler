//
//  strings.h
//  c_compiler
//
//  Created by David Allison on 2/14/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

// Non-standard string functions provided by most OSes.

#ifndef strings_h
#define strings_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int bcmp(const void *, const void *, size_t);
void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);
int ffs(int);
int ffsl(long);
int ffsll(long long);
char *index(const char *, int);
char *rindex(const char *, int);
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);

#ifdef __cplusplus
}
#endif
#endif /* strings_h */
