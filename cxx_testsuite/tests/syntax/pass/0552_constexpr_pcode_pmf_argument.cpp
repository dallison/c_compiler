// RUN: -std=c++20 -fconstexpr-eval=pcode

struct Target {
  int value;

  constexpr int add(int amount) const {
    return value + amount;
  }
};

constexpr int apply(int (Target::*function)(int) const, const Target& object,
                    int amount) {
  return (object.*function)(amount);
}

static_assert(apply(&Target::add, Target{40}, 2) == 42);
