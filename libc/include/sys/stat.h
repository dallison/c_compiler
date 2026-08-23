//
//  stat.h
//  c_compiler
//
//  Created by David Allison on 2/15/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stat_h
#define stat_h

#include <stdint.h>

struct stat {
    uint64_t  st_dev;

    uint64_t       __st_ino;
    uint32_t        st_mode;
    uint32_t        st_nlink;
    uint64_t       st_uid;
    uint64_t       st_gid;
    uint64_t  st_rdev;
    int64_t           st_size;
    uint64_t  st_blksize;
    uint64_t  st_blocks;
    uint64_t       st_atime;
    uint64_t       st_atime_nsec;
    uint64_t       st_mtime;
    uint64_t       st_mtime_nsec;
    uint64_t       st_ctime;
    uint64_t       st_ctime_nsec;
    uint64_t  st_ino;
};

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#ifdef __cplusplus
extern "C" {
#endif

extern int stat(const char* path, struct stat* buf);
extern int fstat(int fd, struct stat* buf);
extern int lstat(const char* path, struct stat* buf);
extern int chmod(const char* path, uint32_t mode);

#ifdef __cplusplus
}
#endif

#endif /* stat_h */
