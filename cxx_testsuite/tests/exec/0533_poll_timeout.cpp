// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <poll.h>

int main() {
  struct pollfd fd;
  fd.fd = -1;
  fd.events = 0;
  fd.revents = 0;
  if (poll(&fd, 0, 0) != 0) {
    return 1;
  }
  return 0;
}
