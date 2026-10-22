#ifndef davecc_net_if_h
#define davecc_net_if_h

#define IF_NAMESIZE 16
#define IFNAMSIZ IF_NAMESIZE
#define IFF_UP 0x1
#define IFF_BROADCAST 0x2
#define IFF_LOOPBACK 0x8
#define IFF_POINTOPOINT 0x10
#define IFF_RUNNING 0x40
#define IFF_MULTICAST 0x1000

unsigned int if_nametoindex(const char* name);
char* if_indextoname(unsigned int index, char* name);

#endif
