// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <utility>

template <class U>
struct Holder {
  template <class... Args>
  explicit Holder(Args&&... args) {
    value = InitializeStorage(std::forward<Args>(args)...);
  }

  template <class... Args>
  static U InitializeStorage(Args&&... args) {
    return U(static_cast<Args&&>(args)...);
  }

  U value;
};

int main() {
  Holder<int> h(7);
  return h.value == 7 ? 0 : 1;
}
