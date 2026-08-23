//
//  errno.h
//  c_compiler
//
//  Created by David Allison on 1/19/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef errno_h
#define errno_h
#ifdef __DAVECC__

#include <syscall.h>

#if defined(__DAVECC_NATIVE_LINUX__)
#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENOEXEC 8
#define EBADF 9
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EDOM 33
#define ERANGE 34
#define EDEADLK 35
#define ENAMETOOLONG 36
#define ENOSYS 38
#define ENOTEMPTY 39
#define ELOOP 40
#define EOVERFLOW 75
#define EILSEQ 84
#define ENOTSUP 95
#define EOPNOTSUPP ENOTSUP
#define ETIMEDOUT 110
#define EUNKNOWN 4095
#else
#define EDOM 200
#define EILSEQ 201
#define ENOENT          1       /* No such file or directory */
#define ENOMEM          2       /* Out of memory */
#define EACCES          3       /* Permission denied */
#define ENODEV          4       /* No such device */
#define EMFILE          5       /* Too many open files */
#define EBUSY           6       /* Device or resource busy */
#define EINVAL          7       /* Invalid argument */
#define ENOSPC          8       /* No space left on device */
#define EEXIST          9       /* File exists */
#define EAGAIN          10      /* Try again */
#define EIO             11      /* I/O error */
#define EINTR           12      /* Interrupted system call */
#define ENOSYS          13      /* Function not implemented */
#define ESPIPE          14      /* Illegal seek */
#define ERANGE          15      /* Range error */
#define EBADF           16      /* Bad file number */
#define ENOEXEC         17      /* Exec format error */
#define EUNKNOWN        18      /* Unknown OS specific error */
#define EPERM           19      /* Operation not permitted */
#define ESRCH           20      /* No such process */
#define EDEADLK         21      /* Resource deadlock would occur */
#define ENOTDIR         22      /* Not a directory */
#define EISDIR          23      /* Is a directory */
#define ENAMETOOLONG    24      /* File name too long */
#define ENOTEMPTY       25      /* Directory not empty */
#define ELOOP           26      /* Too many symbolic links */
#define EROFS           27      /* Read-only file system */
#define EXDEV           28      /* Cross-device link */
#define ENOTSUP         29      /* Operation not supported */
#define EOPNOTSUPP      ENOTSUP
#define EFBIG           30      /* File too large */
#define ENFILE          31      /* Too many open files in system */
#define EOVERFLOW       32      /* Value too large for defined data type */
#define ENOTTY          33      /* Inappropriate ioctl for device */
#endif

#if defined(__6502__)
#define ERRNO_ADDRESS 0x3d6
#define errno (*(int*)ERRNO_ADDRESS)
#elif defined(__DAVECC_DYNAMIC_LIBC__)
#ifdef __cplusplus
extern "C" {
#endif
int* __davecc_errno_location(void);
#ifdef __cplusplus
}
#endif
#define errno (*__davecc_errno_location())
#elif defined(__DAVECC_HAS_TLS_THREAD_ERRNO__)
extern __thread int errno;
#else
extern int errno;
#endif

#endif /* __DAVECC__ */
#endif /* errno_h */
