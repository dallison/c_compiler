// RUN: -std=c++20
// [basic.lookup.argdep]: the associated namespace of a class is its *innermost*
// enclosing namespace only.  `grab` is declared in the enclosing namespace
// `outer`, not in `outer::inner` where the argument type lives, so unqualified
// argument-dependent lookup must NOT find it.
// EXPECT: No matching overload for grab
namespace outer {
namespace inner {
struct S {
  int v;
};
}  // namespace inner

int grab(inner::S s) {
  return s.v;
}
}  // namespace outer

int use(outer::inner::S s) {
  return grab(s);
}
