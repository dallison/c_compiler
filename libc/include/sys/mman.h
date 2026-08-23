//
//  mman.h
//  c_compiler
//
//  Created by David Allison on 2/15/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef mman_h
#define mman_h

#include <stddef.h>
#include <stddef.h>

#ifndef __OFF_T
#define __OFF_T
typedef long off_t;
#endif

#define PROT_READ       0x1             /* Page can be read.  */
#define PROT_WRITE      0x2             /* Page can be written.  */
#define PROT_EXEC       0x4             /* Page can be executed.  */
#define PROT_NONE       0x0             /* Page can not be accessed.  */
#define MAP_SHARED      0x01            /* Share changes.  */
#define MAP_PRIVATE     0x02            /* Changes are private.  */

#define MAP_FIXED       0x10            /* Interpret addr exactly.  */
#define MAP_ANONYMOUS   0x20            /* Don't use a file.  */

#define MAP_ANON  MAP_ANONYMOUS

#define MAP_FAILED ((void *)-1)

#define MREMAP_MAYMOVE  1
#define MREMAP_FIXED    2

#ifdef __cplusplus
extern "C" {
#endif

extern void*  mmap(void *, size_t, int, int, int, off_t);
extern int    munmap(void *, size_t);
extern int    msync(const void *, size_t, int);
extern int    mprotect(const void *, size_t, int);
extern void*  mremap(void *, size_t, size_t, unsigned long);

extern int    mlockall(int);
extern int    munlockall(void);
extern int    mlock(const void *, size_t);
extern int    munlock(const void *, size_t);
extern int    madvise(const void *, size_t, int);

extern int    mlock(const void *addr, size_t len);
extern int    munlock(const void *addr, size_t len);

extern int    mincore(void*  start, size_t  length, unsigned char*  vec);

#ifdef __cplusplus
}
#endif

#endif /* mman_h */
