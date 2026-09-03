// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <clocale>
#include <csignal>
#include <climits>
#include <cstring>

static volatile std::sig_atomic_t observed_signal;

static void handle_signal(int value) {
  observed_signal = value;
}

int main() {
  if (std::setlocale(LC_ALL, 0) == 0 ||
      std::strcmp(std::setlocale(LC_ALL, "C"), "C") != 0 ||
      std::setlocale(LC_ALL, "unsupported") != 0) {
    return 1;
  }

  std::lconv* values = std::localeconv();
  if (values == 0 || std::strcmp(values->decimal_point, ".") != 0 ||
      values->int_frac_digits != CHAR_MAX) {
    return 2;
  }

  if (std::signal(SIGTERM, handle_signal) != SIG_DFL ||
      std::raise(SIGTERM) != 0 || observed_signal != SIGTERM) {
    return 3;
  }
  if (std::signal(SIGTERM, SIG_IGN) != handle_signal ||
      std::raise(SIGTERM) != 0 || observed_signal != SIGTERM) {
    return 4;
  }
  if (std::signal(99, handle_signal) != SIG_ERR || std::raise(99) == 0) {
    return 5;
  }
  return 0;
}
