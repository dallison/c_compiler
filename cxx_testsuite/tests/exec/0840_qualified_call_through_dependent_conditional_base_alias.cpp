// A nested class derived from a dependent alias of an enclosing class
// template's nested classes (`using Inl = conditional_t<N == 0, E, NE>`, as in
// absl::FixedArray) calls `Inl::Get()` on the instantiated nested class, whose
// out-of-line member is defined for that instantiation.
#include <cstddef>
#include <type_traits>

template <typename T, std::size_t N>
struct FixedArray {
  struct NonEmpty {
    int Get();
    T value[N == 0 ? 1 : N];
  };
  struct Empty {
    int Get() { return 0; }
  };
  using Inlined = std::conditional_t<N == 0, Empty, NonEmpty>;
  struct Storage : Inlined {
    int Size() { return Inlined::Get(); }
  };
  using Enabled = std::enable_if_t<(N > 1), NonEmpty>;
  struct Checked : Enabled {
    int Size() { return Enabled::Get() + 1; }
  };
};

template <typename T, std::size_t N>
int FixedArray<T, N>::NonEmpty::Get() {
  return static_cast<int>(N);
}

int main() {
  FixedArray<char, 4>::Storage four;
  FixedArray<char, 0>::Storage none;
  FixedArray<int, 3>::Checked checked;
  if (four.Size() != 4) {
    return 1;
  }
  if (none.Size() != 0) {
    return 2;
  }
  return checked.Size() == 4 ? 0 : 3;
}
