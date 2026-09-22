// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <utility>

template <bool B, class R, class... P>
struct Core {
  template <class QualTRef, class... Args>
  explicit Core(int, Args&&... args) {
    InitializeStorage<QualTRef>(std::forward<Args>(args)...);
  }

  template <class T, class... Args>
  void InitializeStorage(Args&&... args) {
    value = T(static_cast<Args&&>(args)...);
  }

  R value;
};

int main() {
  // The constructor template body is deferred and must parse
  // `InitializeStorage<QualTRef>(std::forward<Args>(args)...)` once the
  // later member template is visible.
  return 0;
}
