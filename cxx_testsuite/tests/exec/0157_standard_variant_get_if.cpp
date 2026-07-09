// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

int main(void) {
  std::variant<int, long> value(11);
  const std::variant<int, long>& const_value = value;

  int* active_int = std::get_if<0>(&value);
  if (active_int == nullptr || *active_int != 11) {
    return 1;
  }
  *active_int = 12;
  if (std::get<int>(value) != 12) {
    return 2;
  }

  const int* const_active_int = std::get_if<int>(&const_value);
  if (const_active_int == nullptr || *const_active_int != 12) {
    return 3;
  }

  if (std::get_if<1>(&value) != nullptr ||
      std::get_if<long>(&value) != nullptr) {
    return 4;
  }

  if (std::get_if<0>(static_cast<std::variant<int, long>*>(nullptr)) !=
          nullptr ||
      std::get_if<int>(static_cast<std::variant<int, long>*>(nullptr)) !=
          nullptr) {
    return 5;
  }

  value = 30L;
  if (std::get_if<0>(&value) != nullptr ||
      std::get_if<int>(&value) != nullptr) {
    return 6;
  }
  long* active_long = std::get_if<long>(&value);
  if (active_long == nullptr || *active_long != 30L) {
    return 7;
  }

  return 0;
}
