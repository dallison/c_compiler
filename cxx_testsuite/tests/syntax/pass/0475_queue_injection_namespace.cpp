// RUN: -std=c++29

#include <meta>

namespace N {
  consteval {
    std::meta::queue_injection(^{ constexpr int x = 42; });
  }
}

static_assert(N::x == 42);

int main() { return 0; }
