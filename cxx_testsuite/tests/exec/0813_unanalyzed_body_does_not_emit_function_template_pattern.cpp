// RUN: -std=c++17
// EXPECT_EXIT: 0

// A non-template inline function whose body is analyzed lazily still names
// an overloaded function template from parse-time lookup.  Collecting
// references from that body must not emit the template pattern itself, whose
// dependent body cannot be compiled.  Abseil's btree string comparator calls
// compare_result_as_ordering this way.

#include <type_traits>

template <typename T>
struct OrderingBase {
  static const T less;
  static const T equivalent;
  static const T greater;
};

class Ordering : public OrderingBase<Ordering> {
  friend struct OrderingBase<Ordering>;
  explicit constexpr Ordering(int v) : value_(v) {}

 public:
  int value_;
};

template <typename T> const T OrderingBase<T>::less(-1);
template <typename T> const T OrderingBase<T>::equivalent(0);
template <typename T> const T OrderingBase<T>::greater(1);

template <typename Int, std::enable_if_t<std::is_same<int, Int>::value, int> = 0>
constexpr Ordering AsOrdering(const Int c) {
  return c < 0 ? Ordering::less : c == 0 ? Ordering::equivalent : Ordering::greater;
}
constexpr Ordering AsOrdering(const Ordering c) { return c; }

struct Less {
  Ordering operator()(int lhs, int rhs) const { return AsOrdering(lhs - rhs); }
};

int main() {
  Ordering r = Less()(1, 3);
  Ordering same = AsOrdering(r);
  return r.value_ == -1 && same.value_ == -1 ? 0 : 1;
}
