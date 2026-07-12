//
//  errno.c
//  libc
//

#if !defined(__6502__)
#if defined(__x86_64__) && !defined(__p_code__)
__thread int errno;
#else
int errno;
#endif
#endif
