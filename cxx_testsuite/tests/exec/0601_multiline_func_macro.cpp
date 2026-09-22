// RUN: -std=c++17
// EXPECT_EXIT: 0

#define ADD(a, b) ((a) + (b))

int main() {
  return ADD(1,
             2) == 3
             ? 0
             : 1;
}
