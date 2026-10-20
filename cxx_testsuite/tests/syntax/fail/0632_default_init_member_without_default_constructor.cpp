// RUN: -std=c++20
// EXPECT: Use of implicitly deleted function Box

struct R {
  R(int) {}
};

struct Box {
  R value;
};

int main() {
  Box b;
  (void)b;
  return 0;
}
