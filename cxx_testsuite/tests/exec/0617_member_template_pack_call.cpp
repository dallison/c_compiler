// RUN: -std=c++17
// EXPECT_EXIT: 0

// A later-declared member function template called with explicit arguments
// and a pack expansion must keep the template-id.  Abseil's CoreImpl does
// `InitializeStorage<QualTRef>(std::forward<Args>(args)...)`.

template <class T>
struct in_place_t {};

template <class Sig>
struct Core {
  template <class QualTRef, class... Args>
  explicit Core(in_place_t<QualTRef>, Args&&... args) {
    InitializeStorage<QualTRef>(static_cast<Args&&>(args)...);
  }

  template <class QualTRef, class... Args>
  void InitializeStorage(Args&&... args) {
    using RawT = QualTRef;
    if constexpr (sizeof(RawT) == sizeof(int)) {
      InitializeRemote<RawT>(static_cast<Args&&>(args)...);
    }
    n = sizeof...(Args);
  }

  template <class T, class... Args>
  void InitializeRemote(Args&&... args) {
    (void)sizeof(T);
    n = (int)sizeof...(Args);
  }

  int n;
};

int main() {
  Core<int> c(in_place_t<int>{}, 1, 2);
  return c.n == 2 ? 0 : 1;
}
