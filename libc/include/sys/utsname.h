//
//  utsname.h
//  c_compiler
//

#ifndef sys_utsname_h
#define sys_utsname_h

#include <davecc_guest_syscalls.h>

#ifdef __cplusplus
extern "C" {
#endif

// Field width matches DAVE_UTSNAME_LENGTH.  The interpreter copies the host
// uname result into this layout; it is not the host's struct utsname.
struct utsname {
  char sysname[DAVE_UTSNAME_LENGTH];
  char nodename[DAVE_UTSNAME_LENGTH];
  char release[DAVE_UTSNAME_LENGTH];
  char version[DAVE_UTSNAME_LENGTH];
  char machine[DAVE_UTSNAME_LENGTH];
};

int uname(struct utsname* name);

#ifdef __cplusplus
}
#endif

#endif
