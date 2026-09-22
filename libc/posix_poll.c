#include <poll.h>
#include <syscall.h>

#if defined(__DAVECC_NATIVE_DARWIN__)
// poll comes from libSystem.
#else
int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  return (int)syscall(SYS_POLL, fds, nfds, timeout);
}
#endif
