// RUN: -std=c++17
// EXPECT_EXIT: 0

// An out-of-line definition of a static data member may spell its type as a
// cv-qualified dependent member of its own class, as absl::Span does for npos.
// The qualifiers stay part of the type, including for a class template with
// a partial specialization.

template <class T>
struct S {
  using size_type = unsigned long;
  static const size_type npos = ~size_type(0);
  size_type get(size_type v = npos) const { return v; }
};
template <class T>
const typename S<T>::size_type S<T>::npos;

template <class T>
struct P {
  using size_type = unsigned long;
  static const size_type npos = ~size_type(0);
};
template <class T>
struct P<T*> {
  using size_type = unsigned;
  static const size_type npos = 7;
};
template <class T>
const typename P<T>::size_type P<T>::npos;

int main() {
  S<int> s;
  if (s.get() != ~0ul) return 1;
  if (P<int>::npos != ~0ul) return 2;
  if (P<int*>::npos != 7) return 3;
  const unsigned long* a = &S<int>::npos;
  const unsigned long* b = &P<int>::npos;
  return (*a == ~0ul && *b == ~0ul) ? 0 : 4;
}
