#ifndef davecc_sys_socket_h
#define davecc_sys_socket_h

#if defined(__DAVECC_NATIVE_DARWIN__)
// Native code calls libSystem, so use the SDK's declarations and layouts.
#include <sys/types.h>
#include_next <sys/socket.h>
#else

#include <stdint.h>

#define AF_UNSPEC 0
#define SOCK_STREAM 1

typedef unsigned int socklen_t;

struct sockaddr {
  unsigned char sa_data[16];
};

#ifdef __cplusplus
extern "C" {
#endif

int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC_NATIVE_DARWIN__ */
#endif
