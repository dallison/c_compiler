// RUN: -std=c++20
// EXPECT: alias declaration in init-statement requires C++23

int main() {
  if (using value_type = int; true) {
    return value_type{0};
  }
  return 1;
}
