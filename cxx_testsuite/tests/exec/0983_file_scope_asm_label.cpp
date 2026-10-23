// RUN: -std=c++17
// TARGETS: x86_64
// EXPECT_EXIT: 0

extern "C" int forty_two(void);

asm(
    "forty_two:\n"
    "movl $42, %eax\n"
    "ret\n");

int main() {
  return forty_two() == 42 ? 0 : 1;
}
