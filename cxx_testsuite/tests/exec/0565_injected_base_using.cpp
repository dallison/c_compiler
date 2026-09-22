// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class H>
struct HashStateBase {
  static int combine_contiguous() { return 7; }
};

struct State : HashStateBase<State> {
  using State::HashStateBase::combine_contiguous;
};

int main() { return State::combine_contiguous() == 7 ? 0 : 1; }
