// RUN: -std=c++23
// EXPECT: 'indeterminate' attribute requires C++26

void rejected() {
  [[indeterminate]] int value;
  (void)&value;
}
