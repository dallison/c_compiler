// RUN: -std=c++29

#include <meta>

void invalid_runtime_injection() {
  std::meta::queue_injection(^{ int runtime_injected; });
}
