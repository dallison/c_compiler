// RUN: -std=c++20
// EXPECT: Template non-type argument is not compatible with parameter type

int returns_int(int value) {
  return value;
}

template <void (*Function)(int)>
struct Consumer {};

Consumer<&returns_int> invalid;
