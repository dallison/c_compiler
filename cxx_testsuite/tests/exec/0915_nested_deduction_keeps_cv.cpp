// Only a by-value call parameter drops the argument's top-level cv during
// deduction.  A placeholder nested in a function type's return type or a
// member pointer keeps it, and the selected specialization must still accept
// the argument it was deduced from.  A prvalue of cv-qualified scalar type has
// the unqualified type ([expr.type]), which decltype reports.  gmock deduces
// MOCK_METHOD argument types (`(const int)`) this way.
template <class A, class B> struct same { static constexpr bool value = false; };
template <class A> struct same<A, A> { static constexpr bool value = true; };

template <class R> R call(R (*fn)()) { return fn(); }
template <class T, class C> T read(T C::*member, const C& object) {
  return object.*member;
}
template <class R> struct Returns { using type = R; };
template <class R> Returns<R> returns(R (*)());

const int seven() { return 7; }
int eight() { return 8; }
struct Box {
  const int fixed;
  int plain;
};

int main() {
  int failures = 0;
  if (call(&seven) != 7 || call(&eight) != 8) failures |= 1;
  // R itself is `const int`; the call expression's type is `int`.
  static_assert(same<decltype(returns(&seven))::type, const int>::value, "");
  static_assert(same<decltype(call(&seven)), int>::value, "");
  static_assert(same<decltype(seven()), int>::value, "");
  decltype(seven()) assignable = 1;
  assignable = 2;
  if (assignable != 2) failures |= 2;
  Box box{3, 4};
  if (read(&Box::fixed, box) != 3 || read(&Box::plain, box) != 4) failures |= 4;
  static_assert(same<decltype(read(&Box::fixed, box)), int>::value, "");
  return failures;
}
