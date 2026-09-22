// RUN: -std=c++17
// EXPECT_EXIT: 0

namespace inner {

constexpr bool Pred(void*, void*) { return true; }

template <class T>
struct Outer {
  template <class Other>
  struct Trait {
    static constexpr bool value = false;
  };

  template <class U>
  struct Trait<U*> {
    static constexpr bool value =
        (Pred)(static_cast<void*>(nullptr), static_cast<void*>(nullptr));
  };
};

}  // namespace inner

int main() {
  return inner::Outer<int>::Trait<int*>::value ? 0 : 1;
}
