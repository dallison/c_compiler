// RUN: -std=c++26 -fexceptions

#include <format>

inline std::string format_integer(int value) {
  return std::format("{}", value);
}
