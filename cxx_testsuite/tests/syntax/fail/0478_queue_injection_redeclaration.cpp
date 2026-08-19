// RUN: -std=c++29

#include <meta>

namespace duplicate {
constexpr int value = 1;
consteval {
  std::meta::queue_injection(^{ constexpr int value = 2; });
}
}
