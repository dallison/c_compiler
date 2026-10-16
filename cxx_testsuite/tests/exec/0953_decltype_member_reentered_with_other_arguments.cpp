// RUN: -std=c++17
// EXPECT_EXIT: 0

// [temp.inst]: instantiating `result_of<const Checked(const int&, const int&)>`
// resolves `Checked::operator()`, whose constraint instantiates the same
// partial specialization -- the same decltype operand -- for `Less`.  That
// is a different specialization, not a recursion (abseil's checked_compare
// under C++17 std::result_of).
template <class T>
T&& declval();

template <bool B, class T = void>
struct enable_if {};
template <class T>
struct enable_if<true, T> {
  using type = T;
};

template <class>
struct result_of;
template <class F, class... A>
struct result_of<F(A...)> {
  using type = decltype(declval<F>()(declval<A>()...));
};

template <class X, class Y>
struct same {
  static constexpr bool value = false;
};
template <class X>
struct same<X, X> {
  static constexpr bool value = true;
};

struct Less {
  bool operator()(const int& a, const int& b) const { return a < b; }
};

template <class Compare>
struct Checked : Compare {
  template <class T, class U,
            typename enable_if<
                same<bool, typename result_of<const Compare(
                               const T&, const U&)>::type>::value,
                int>::type = 0>
  bool operator()(const T& l, const U& r) const {
    return Compare::operator()(l, r);
  }
};

template <class Compare>
struct Twice : Compare {
  template <class T,
            class R = typename result_of<const Compare(const T&, const T&)>::type>
  R operator()(const T& l, const T& r) const {
    return Compare::operator()(l, r) && Compare::operator()(l, r);
  }
};

static_assert(
    same<result_of<const Checked<Less>(const int&, const int&)>::type,
         bool>::value,
    "result_of through a constrained call operator");
static_assert(
    same<result_of<const Twice<Checked<Less>>(const int&, const int&)>::type,
         bool>::value,
    "nested twice");

int main() {
  Checked<Less> c;
  Twice<Checked<Less>> t;
  return c(1, 2) && !c(2, 1) && t(1, 2) ? 0 : 1;
}
