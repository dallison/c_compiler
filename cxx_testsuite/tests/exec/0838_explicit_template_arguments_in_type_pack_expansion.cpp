// A pack expansion whose pattern names the type pack only through `sizeof(T)`
// still substitutes the explicit template arguments of calls in the pattern
// (protobuf FlatAllocation: `RoundUpTo<kMaxAlign>(sizeof(T))...`).
#include <cstddef>

namespace {
template <int R>
constexpr size_t RoundUpTo(size_t n) {
  return (n + (R - 1)) & ~(R - 1);
}

constexpr size_t Max(size_t a, size_t b) { return a > b ? a : b; }

template <typename... T>
struct FlatAllocation {
  static constexpr size_t kMaxAlign = Max(alignof(T)...);
  size_t ends[sizeof...(T)];
  FlatAllocation() {
    size_t i = 0;
    int expand[] = {((ends[i++] = RoundUpTo<kMaxAlign>(sizeof(T) + 1)), 0)...};
    (void)expand;
  }
};

template <typename U, typename... T>
struct Outer {
  size_t sizes[sizeof...(T)];
  Outer() {
    size_t i = 0;
    int expand[] = {((sizes[i++] = RoundUpTo<sizeof(U)>(sizeof(T) + 1)), 0)...};
    (void)expand;
  }
};
}  // namespace

int main() {
  FlatAllocation<char, double> f;
  if (f.ends[0] != 8 || f.ends[1] != 16) return 1;
  Outer<int, char, short, double> o;
  if (o.sizes[0] != 4 || o.sizes[1] != 4 || o.sizes[2] != 12) return 2;
  return 0;
}
