#ifndef davecc_arpa_inet_h
#define davecc_arpa_inet_h

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

#ifdef __cplusplus
}
#endif

#endif

#endif
