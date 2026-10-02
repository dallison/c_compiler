//
//  fcntl.h
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef fcntl_h
#define fcntl_h


#ifndef __SIZE_T
#if defined(__6502__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#ifndef __SSIZE_T
#if defined(__6502__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define __SSIZE_T
#endif

#ifndef __FPOS_T
typedef long fpos_t;
#define __FPOS_T
#endif

#define O_ACCMODE 00000003
#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002
#if defined(__DAVECC_NATIVE_DARWIN__)
/* Values match Darwin's <sys/fcntl.h>; native code calls libSystem. */
#define O_NONBLOCK 0x00000004
#define O_APPEND 0x00000008
#define O_SHLOCK 0x00000010
#define O_EXLOCK 0x00000020
#define O_ASYNC 0x00000040
#define FASYNC O_ASYNC
#define O_SYNC 0x00000080
#define O_FSYNC O_SYNC
#define O_NOFOLLOW 0x00000100
#define O_CREAT 0x00000200
#define O_TRUNC 0x00000400
#define O_EXCL 0x00000800
#define O_NOCTTY 0x00020000
#define O_DIRECTORY 0x00100000
#define O_SYMLINK 0x00200000
#define O_DSYNC 0x00400000
#define O_CLOEXEC 0x01000000
#define O_NDELAY O_NONBLOCK

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_GETOWN 5
#define F_SETOWN 6
#define F_GETLK 7
#define F_SETLK 8
#define F_SETLKW 9
#define F_GETPATH 50
#define F_FULLFSYNC 51
#define F_DUPFD_CLOEXEC 67

#define FD_CLOEXEC 1

#define F_RDLCK 1
#define F_UNLCK 2
#define F_WRLCK 3

#define AT_FDCWD -2
#define AT_EACCESS 0x0010
#define AT_SYMLINK_NOFOLLOW 0x0020
#define AT_SYMLINK_FOLLOW 0x0040
#define AT_REMOVEDIR 0x0080
#else
#ifndef O_CREAT
#define O_CREAT 00000100
#endif
#ifndef O_EXCL
#define O_EXCL 00000200
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 00000400
#endif
#ifndef O_TRUNC
#define O_TRUNC 00001000
#endif
#ifndef O_APPEND
#define O_APPEND 00002000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 00004000
#endif
#ifndef O_SYNC
#define O_SYNC 00010000
#endif
#ifndef FASYNC
#define FASYNC 00020000
#endif
#ifndef O_DIRECT
#define O_DIRECT 00040000
#endif
#ifndef O_LARGEFILE
#define O_LARGEFILE 00100000
#endif
#ifndef O_DIRECTORY
#define O_DIRECTORY 00200000
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 00400000
#endif
#ifndef O_NOATIME
#define O_NOATIME 01000000
#endif
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#ifndef F_GETLK
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7
#endif
#ifndef F_SETOWN
#define F_SETOWN 8
#define F_GETOWN 9
#endif
#ifndef F_SETSIG
#define F_SETSIG 10
#define F_GETSIG 11
#endif

#define FD_CLOEXEC 1

#ifndef F_RDLCK
#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2
#endif

#ifndef F_EXLCK
#define F_EXLCK 4
#define F_SHLCK 8
#endif

#ifndef F_INPROGRESS
#define F_INPROGRESS 16
#endif
#endif /* __DAVECC_NATIVE_DARWIN__ */

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8

#define LOCK_MAND 32
#define LOCK_READ 64
#define LOCK_WRITE 128
#define LOCK_RW 192  

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
int open(const char* filename, int mode, ...);
int close(int fd);
int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif /* fcntl_h */
