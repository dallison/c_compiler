// RUN: -std=c++17
// EXPECT_EXIT: 0

class MixingHashState {
  unsigned state_;
  MixingHashState() : state_(1) {}
  explicit MixingHashState(unsigned state) : state_(state) {}

  static MixingHashState run(MixingHashState inner) {
    unsigned unordered = 0;
    auto take = [&](MixingHashState& inner_state) {
      auto element_state = inner_state.state_;
      unordered += element_state;
      inner_state = MixingHashState{};
    };
    take(inner);
    return MixingHashState(unordered);
  }

 public:
  static int go() {
    MixingHashState s(3);
    return run(s).state_ == 3 ? 0 : 1;
  }
};

int main() { return MixingHashState::go(); }
