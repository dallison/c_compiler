// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <stdfloat>
#include <typeinfo>

std::float32_t input32 = 1.25f32;
std::float64_t input64 = 2.5f64;

float overload(float value) { return value + 10.0f; }
std::float32_t overload(std::float32_t value) { return value + 20.0f32; }
double overload(double value) { return value + 30.0; }
std::float64_t overload(std::float64_t value) { return value + 40.0f64; }

std::float32_t add32(std::float32_t left, std::float32_t right) {
  return left + right;
}

std::float64_t add64(std::float64_t left, std::float64_t right) {
  return left + right;
}

template <class T>
T twice(T value) {
  return value + value;
}

int main() {
  if (overload(input32) != 21.25f32) {
    return 1;
  }
  if (overload(input64) != 42.5f64) {
    return 2;
  }
  if (add32(input32, 0.75f32) != 2.0f32) {
    return 3;
  }
  if (add64(input64, 1.5f64) != 4.0f64) {
    return 4;
  }
  if (twice(input32) != 2.5f32 || twice(input64) != 5.0f64) {
    return 5;
  }

  float standard32 = input32;
  double standard64 = input64;
  if (static_cast<std::float32_t>(standard32) != input32 ||
      static_cast<std::float64_t>(standard64) != input64) {
    return 6;
  }
  if (typeid(std::float32_t) == typeid(float) ||
      typeid(std::float64_t) == typeid(double)) {
    return 7;
  }
  return 0;
}
