#ifndef davecc_netinet_in_h
#define davecc_netinet_in_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <netinet/in.h>
#else

#include <stdint.h>
#include <sys/socket.h>

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
  in_addr_t s_addr;
};

struct sockaddr_in {
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
  unsigned char sin_zero[8];
};

#define INADDR_ANY ((in_addr_t)0)
#define INET_ADDRSTRLEN 16

#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

#define IP_ADD_MEMBERSHIP 35
#define IP_DROP_MEMBERSHIP 36
#define IP_MULTICAST_LOOP 34

struct ip_mreq {
  struct in_addr imr_multiaddr;
  struct in_addr imr_interface;
};

#ifdef __cplusplus
extern "C" {
#endif

uint16_t htons(uint16_t value);
uint16_t ntohs(uint16_t value);
uint32_t htonl(uint32_t value);
uint32_t ntohl(uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
#endif
