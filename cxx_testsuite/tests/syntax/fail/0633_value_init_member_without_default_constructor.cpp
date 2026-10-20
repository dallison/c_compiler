// RUN: -std=c++20
// EXPECT: No matching overload for R

struct R {
  R(int) {}
};

struct Box {
  R value;
};

int main() {
  Box b = Box();
  (void)b;
  return 0;
}
