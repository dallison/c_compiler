// RUN: -std=c++20

#include <any>

void invalid_pointer_cast(std::any* value) {
  (void)std::any_cast<void>(value);
}
