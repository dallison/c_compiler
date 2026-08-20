// RUN: -std=c++29
// EXPECT: Use of implicitly deleted function

struct counter {
  counter operator++(int) = default;
};

void use_deleted_postfix() {
  counter value;
  value++;
}
