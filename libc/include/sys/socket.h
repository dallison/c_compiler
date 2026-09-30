#ifndef davecc_sys_socket_h
#define davecc_sys_socket_h

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

#endif
