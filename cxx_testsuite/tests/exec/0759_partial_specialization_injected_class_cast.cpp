// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class Signature>
struct Wrapper;

template <class Return, class... Args>
struct Wrapper<Return(Args...) const> {
  int value;

  void Set(int next) const {
    [this, next] { const_cast<Wrapper*>(this)->value = next; }();
  }
};

int main() {
  Wrapper<void(int) const> wrapper{1};
  wrapper.Set(7);
  return wrapper.value == 7 ? 0 : 1;
}
