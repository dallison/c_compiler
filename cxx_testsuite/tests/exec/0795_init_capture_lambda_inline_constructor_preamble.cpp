// RUN: -std=c++17
// EXPECT_EXIT: 0

// The lambda body is analyzed while the class's member bodies are still being
// parsed.  Inlining `Hash{}` there must not analyze the default constructor
// before its member initializer is inserted into its body.
class Hash {
  unsigned state_;
  Hash() : state_(1) {}
  explicit Hash(unsigned state) : state_(state) {}

  static Hash run(Hash inner) {
    unsigned total = 0;
    auto take = [&, extra = 0u](Hash& inner_state) {
      total += inner_state.state_ + extra;
      inner_state = Hash{};
    };
    take(inner);
    return Hash(total + inner.state_);
  }

 public:
  static int go() {
    Hash h(3);
    return run(h).state_ == 4 ? 0 : 1;
  }
};

int main() { return Hash::go(); }
