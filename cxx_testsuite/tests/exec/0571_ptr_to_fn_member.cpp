// RUN: -std=c++17
// EXPECT_EXIT: 0

struct HashState;

template <class F>
struct FunctionRef {
  F* fn;
};

static HashState bounce(HashState state,
                        FunctionRef<void(HashState, FunctionRef<void(HashState&)>)>);

struct HashState {
  void* state_;
  HashState (*run_combine_unordered_)(
      HashState state,
      FunctionRef<void(HashState, FunctionRef<void(HashState&)>)>);

  void Init(HashState* other) {
    state_ = other->state_;
    run_combine_unordered_ = other->run_combine_unordered_;
  }
};

static HashState bounce(HashState state,
                        FunctionRef<void(HashState, FunctionRef<void(HashState&)>)>) {
  return state;
}

int main() {
  HashState a{};
  a.state_ = &a;
  a.run_combine_unordered_ = &bounce;
  HashState b{};
  b.Init(&a);
  return b.run_combine_unordered_ == &bounce && b.state_ == a.state_ ? 0 : 1;
}
