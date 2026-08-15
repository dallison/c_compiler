// RUN: -std=c++26

#include <exception>

consteval std::exception_ptr capture_exception() {
  try {
    throw 42;
  } catch (...) {
    return std::current_exception();
  }
}

constexpr std::exception_ptr escaped = capture_exception();
