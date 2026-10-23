#ifndef davecc_sys_select_h
#define davecc_sys_select_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <sys/select.h>
#else

#include <sys/time.h>

#define FD_SETSIZE 1024

typedef struct {
  unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
} fd_set;

static inline void FD_ZERO(fd_set* set) {
  for (unsigned i = 0; i < sizeof(set->fds_bits) / sizeof(set->fds_bits[0]);
       i++) {
    set->fds_bits[i] = 0;
  }
}

static inline void FD_SET(int fd, fd_set* set) {
  if (fd < 0 || fd >= FD_SETSIZE) return;
  set->fds_bits[fd / (8 * (int)sizeof(unsigned long))] |=
      1UL << (fd % (8 * (int)sizeof(unsigned long)));
}

static inline void FD_CLR(int fd, fd_set* set) {
  if (fd < 0 || fd >= FD_SETSIZE) return;
  set->fds_bits[fd / (8 * (int)sizeof(unsigned long))] &=
      ~(1UL << (fd % (8 * (int)sizeof(unsigned long))));
}

static inline int FD_ISSET(int fd, const fd_set* set) {
  if (fd < 0 || fd >= FD_SETSIZE) return 0;
  return (set->fds_bits[fd / (8 * (int)sizeof(unsigned long))] &
          (1UL << (fd % (8 * (int)sizeof(unsigned long))))) != 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
           struct timeval* timeout);

#ifdef __cplusplus
}
#endif

#endif
#endif
