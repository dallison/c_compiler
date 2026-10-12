// RUN: -std=c++17
// EXPECT_EXIT: 0

constexpr auto section_value asm("section_value_impl")
    __attribute__((section(".rodata.special"))) = 42;

int main() {
  return section_value == 42 ? 0 : 1;
}
