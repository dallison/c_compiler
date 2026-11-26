#ifndef davecc_arpa_inet_h
#define davecc_arpa_inet_h

#include <stdint.h>
#include <sys/socket.h>

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <arpa/inet.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

int inet_pton(int family, const char* text, void* destination);
const char* inet_ntop(int family, const void* source, char* destination,
                      unsigned int size);

/*
 * DaveOS host-endian IPv4 helpers (a.b.c.d ↔ uint32 with a in the high byte).
 * Distinct from inet_pton/inet_ntop, which use network-order binary.
 */
int inet_ptoa(const char* presentation, uint32_t* addr);
const char* inet_atop(uint32_t addr, char* destination, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif

#endif
