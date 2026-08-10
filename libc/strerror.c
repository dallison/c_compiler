//
//  strerror.c
//  c_compiler
//
//  Created by David Allison on 6/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <string.h>
#include <errno.h>

char* strerror(int errnum) {
  switch (errnum) {
    case EDOM: return "Domain error";
    case EILSEQ: return "Illegal sequence";

    case ENOENT: return "No such file or directory";
    case ENOMEM: return "Out of memory";
    case EACCES: return "Permission denied";
    case ENODEV: return "No such device";
    case EMFILE: return "Too many open files";
    case EBUSY: return  "Device or resource busy";
    case EINVAL: return "Invalid argument";
    case ENOSPC: return "No space left on device";
    case EEXIST: return "File exists";
    case EAGAIN: return "Try again";
    case EIO: return "I/O error";
    case EINTR: return  "Interrupted system call";
    case ENOSYS: return "Function not implemented";
    case ESPIPE: return "Illegal seek";
    case ERANGE: return  "Range error";
    case EBADF: return  "Bad file number";
    case ENOEXEC: return  "Exec format error";
    case EUNKNOWN: return  "Unknown OS specific error";
    case EPERM: return "Operation not permitted";
    case ESRCH: return "No such process";
    case EDEADLK: return "Resource deadlock would occur";
    case ENOTDIR: return "Not a directory";
    case EISDIR: return "Is a directory";
    case ENAMETOOLONG: return "File name too long";
    case ENOTEMPTY: return "Directory not empty";
    case ELOOP: return "Too many symbolic links";
    case EROFS: return "Read-only file system";
    case EXDEV: return "Cross-device link";
    case ENOTSUP: return "Operation not supported";
    case EFBIG: return "File too large";
    case ENFILE: return "Too many open files in system";
    default: {
#if defined(__DAVECC_HAS_TLS_THREAD_ERRNO__)
      static __thread char buf[32];
#else
      static char buf[32];
#endif
      char* s = "Unknown error: ";
      char* p = buf;
      while (*s != '\0') {
        *p++ = *s++;
      }
      char buf2[10];
      s = &buf2[sizeof(buf2) - 1];
      *s-- = '\0';
      if (errnum == 0) {
        *s = '0';
      } else {
        while (errnum != 0) {
          char ch = (errnum % 10) + '0';
          *s-- = ch;
          errnum /= 10;
        }
        // We've gone one too far.
        s++;
      }
      // Append the number in decimal to the end of the buffer.
      while (*s != '\0') {
        *p++ = *s++;
      }
      *p++ = '\0';
      return buf;
    }
  }
}
