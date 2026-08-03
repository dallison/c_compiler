// RUN: -std=c++20 -fconstexpr-eval=pcode

struct member_pointer_target {
  int value;

  constexpr int add(int amount) const {
    return value + amount;
  }
};

constexpr int invoke_member_pointer() {
  member_pointer_target object = {40};
  int (member_pointer_target::*function)(int) const =
      &member_pointer_target::add;
  return (object.*function)(2);
}

static_assert(invoke_member_pointer() == 42);
