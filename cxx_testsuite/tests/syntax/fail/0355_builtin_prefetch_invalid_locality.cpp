// RUN: -std=c++20
// EXPECT: __builtin_prefetch argument is outside its valid range

int data;

int main(void) {
  __builtin_prefetch(&data, 0, 4);
}
