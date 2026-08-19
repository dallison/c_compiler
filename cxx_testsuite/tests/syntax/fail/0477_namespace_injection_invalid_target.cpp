// RUN: -std=c++29

#include <meta>

constexpr int not_a_namespace = 0;

consteval {
  std::meta::namespace_inject(^^not_a_namespace, ^{ int rejected; });
}
