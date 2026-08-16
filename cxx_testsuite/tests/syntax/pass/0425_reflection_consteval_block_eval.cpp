// RUN: -std=c++26

constexpr int sequenced() {
  consteval {
    static_assert(true);
  }
  return 1;
}

static_assert(sequenced() == 1);
