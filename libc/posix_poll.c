#include <poll.h>
#include <syscall.h>

#if defined(__DAVECC_NATIVE_DARWIN__)
// poll comes from libSystem.
#elif defined(__DAVECC_NATIVE_LINUX__) && !defined(SYS_poll)
// The generic Linux syscall table (aarch64, RISC-V) has only ppoll.  RV32
// postdates the time64 transition and implements only ppoll_time64.  Both
// take a 64-bit timespec on every target that reaches this path.
#if defined(__risc_v__) && defined(__ILP32__)
#define __DAVECC_SYS_PPOLL SYS_ppoll_time64
#else
#define __DAVECC_SYS_PPOLL SYS_ppoll
#endif
struct __davecc_kernel_timespec {
  long long tv_sec;
  long long tv_nsec;
};

int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  struct __davecc_kernel_timespec ts;
  struct __davecc_kernel_timespec* tsp = 0;
  if (timeout >= 0) {
    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (long long)(timeout % 1000) * 1000000;
    tsp = &ts;
  }
  return (int)syscall(__DAVECC_SYS_PPOLL, fds, nfds, tsp, 0, 8);
}
#else
int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  return (int)syscall(SYS_POLL, fds, nfds, timeout);
}
#endif
