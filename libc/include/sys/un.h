#ifndef davecc_sys_un_h
#define davecc_sys_un_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <sys/un.h>
#else

#include <sys/socket.h>

struct sockaddr_un {
  sa_family_t sun_family;
  char sun_path[108];
};

#endif
#endif
