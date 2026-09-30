#ifndef davecc_netdb_h
#define davecc_netdb_h

#include <sys/socket.h>

struct addrinfo {
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  socklen_t ai_addrlen;
  struct sockaddr* ai_addr;
  struct addrinfo* ai_next;
};

#ifdef __cplusplus
extern "C" {
#endif

int getaddrinfo(const char* node, const char* service,
                const struct addrinfo* hints, struct addrinfo** res);
void freeaddrinfo(struct addrinfo* res);
const char* gai_strerror(int errcode);

#ifdef __cplusplus
}
#endif

#endif
