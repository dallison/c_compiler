// RUN: -std=c++20
// EXPECT_EXIT: 0

// A lambda inside a member of a partial specialization names the partial's
// own parameter `C` (index 0), not the primary's first argument `E`.
template <class T, class C>
struct F;

struct E {
  int v;
};

template <class C>
struct F<E, C> {
  constexpr int parse(const C* p) const {
    auto is_lt = [](C value) constexpr {
      C open = static_cast<C>('<');
      return value == open && sizeof(C) == sizeof(char);
    };
    return is_lt(*p) ? 1 : 2;
  }
};

static_assert(F<E, char>{}.parse("<") == 1);

int main() {
  F<E, char> f;
  char lt = '<';
  char gt = '>';
  if (f.parse(&lt) != 1) {
    return 1;
  }
  if (f.parse(&gt) != 2) {
    return 2;
  }
  return 0;
}
