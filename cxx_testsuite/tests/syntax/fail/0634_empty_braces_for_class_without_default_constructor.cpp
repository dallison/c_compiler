// RUN: -std=c++20
// EXPECT: No matching overload for R

struct R {
  R(int) {}
};

int main() {
  R r{};
  (void)r;
  return 0;
}
