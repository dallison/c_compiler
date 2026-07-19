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

#if defined(__6502__)
#define ERRNO_ADDRESS 0x3d6
#define errno (*(int*)ERRNO_ADDRESS)
#elif defined(__DAVECC_HAS_TLS_THREAD_ERRNO__)
extern __thread int errno;
#else
extern int errno;
#endif

#endif /* __DAVECC__ */
#endif /* errno_h */
