//
//  types.h
//  libc
//
//  Minimal POSIX <sys/types.h>.  Typedefs match the ones already provided by
//  <unistd.h> and <stddef.h> so the two headers can be included in either order.
//

#ifndef sys_types_h
#define sys_types_h

#include <stddef.h>

#if defined(__DAVECC_NATIVE_DARWIN__)
// Native code calls libSystem, and SDK headers expect every BSD type from the
// SDK's <sys/types.h>.  DaveCC's other headers define the same types with
// matching definitions; these guards stop them repeating the typedefs.
#include_next <sys/types.h>
#define __pid_t
#define __uid_t
#define __gid_t
#define __OFF_T
#define __SSIZE_T
#define __mode_t
#else

#ifndef __pid_t
#define __pid_t
typedef int pid_t;
#endif

#ifndef __uid_t
#define __uid_t
typedef int uid_t;
#endif

#ifndef __gid_t
#define __gid_t
typedef int gid_t;
#endif

#ifndef __OFF_T
#define __OFF_T
typedef long off_t;
#endif

#ifndef __SSIZE_T
#if defined(__6502__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define __SSIZE_T
#endif

#ifndef __mode_t
#define __mode_t
#if defined(__6502__)
typedef char mode_t;
#else
typedef int mode_t;
#endif
#endif

#endif /* __DAVECC_NATIVE_DARWIN__ */

#endif /* sys_types_h */
