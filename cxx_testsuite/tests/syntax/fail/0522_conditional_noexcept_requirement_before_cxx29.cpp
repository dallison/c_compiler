// RUN: -std=c++26
// EXPECT: Conditional noexcept compound requirements require C++29

template <typename F>
concept nothrow_callable = requires(F function) {
  { function() } noexcept(true);
};

int main(void) {
  return 0;
}
