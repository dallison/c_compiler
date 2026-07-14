// RUN: -std=c++20

#include <any>

int& invalid_reference_cast(const std::any& value) {
  return std::any_cast<int&>(value);
}
