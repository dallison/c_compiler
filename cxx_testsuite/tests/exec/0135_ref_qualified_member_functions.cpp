// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <utility>

struct Probe {
  int select() & {
    return 1;
  }

  int select() const& {
    return 2;
  }

  int select() && {
    return 3;
  }

  int select() const&& {
    return 4;
  }
};

struct LValueOnly {
  int select() & {
    return 5;
  }
};

struct RValueOnly {
  int select() && {
    return 6;
  }
};

int main(void) {
  Probe probe;
  const Probe const_probe;
  if (probe.select() != 1) {
    return 1;
  }
  if (const_probe.select() != 2) {
    return 2;
  }
  if (std::move(probe).select() != 3) {
    return 3;
  }
  if (std::move(const_probe).select() != 4) {
    return 4;
  }

  LValueOnly lvalue_only;
  if (lvalue_only.select() != 5) {
    return 5;
  }

  RValueOnly rvalue_only;
  if (std::move(rvalue_only).select() != 6) {
    return 6;
  }

  return 0;
}
