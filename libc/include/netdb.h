#ifndef davecc_netdb_h
#define davecc_netdb_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <netdb.h>
#else

#include <netinet/in.h>
#include <sys/socket.h>

struct hostent {
  char* h_name;
  char** h_aliases;
  int h_addrtype;
  int h_length;
  char** h_addr_list;
};

#define h_addr h_addr_list[0]

struct addrinfo {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  socklen_t ai_addrlen;
  struct sockaddr* ai_addr;
  char* ai_canonname;
  struct addrinfo* ai_next;
};

#define EAI_NONAME -2
#define EAI_FAIL -4

#ifdef __cplusplus
extern "C" {
#endif

struct hostent* gethostbyname(const char* name);
int getaddrinfo(const char* node, const char* service,
                const struct addrinfo* hints, struct addrinfo** result);
void freeaddrinfo(struct addrinfo* result);
const char* gai_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC_NATIVE_DARWIN__ */

#endif
