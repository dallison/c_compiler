// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Token {
  int value;
};

template <typename H>
H mix_token(H hash_state, Token value) {
  return hash_state + value.value;
}

class MixingHashState {
  int state_;

  template <typename H>
  friend H ::mix_token(H, Token);

 public:
  explicit MixingHashState(int state) : state_(state) {}
  operator int() const { return state_; }
  MixingHashState operator+(int n) const { return MixingHashState(state_ + n); }
};

int main() {
  MixingHashState s(4);
  Token t{3};
  MixingHashState out = mix_token(s, t);
  return static_cast<int>(out) == 7 ? 0 : 1;
}
