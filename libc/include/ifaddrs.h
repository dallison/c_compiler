#ifndef davecc_ifaddrs_h
#define davecc_ifaddrs_h

#include <netinet/in.h>
#include <sys/socket.h>

struct ifaddrs {
  struct ifaddrs* ifa_next;
  char* ifa_name;
  unsigned int ifa_flags;
  struct sockaddr* ifa_addr;
  struct sockaddr* ifa_netmask;
  struct sockaddr* ifa_dstaddr;
  void* ifa_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int getifaddrs(struct ifaddrs** list);
void freeifaddrs(struct ifaddrs* list);

#ifdef __cplusplus
}
#endif

#endif
