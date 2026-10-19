// RUN: -std=c++17
// EXPECT_EXIT: 0
// A pack expansion whose pattern calls a member function template with the
// pack as an explicit template argument (`Size<T>()...`, as in protobuf's
// FlatAllocatorImpl::ExpectConsumed).  Each element must call the
// specialization for its own type.
#include <initializer_list>

struct Eater {
  template <typename T> Eater(T&&) {}
};
void Fold(std::initializer_list<Eater>) {}

template <class... A> int Sum(A... a) { return (0 + ... + a); }

template <typename... T> struct Impl {
  int seen = 0;
  int Direct() const { return Sum(Size<T>()...); }
  int ThroughThis() const { return Sum(this->template Size<T>()...); }
  void Mark() { Fold({Record<T>()...}); }
  template <typename U> int Size() const { return (int)sizeof(U); }
  template <typename U> bool Record() {
    seen = seen * 10 + (int)sizeof(U);
    return true;
  }
};

int main() {
  Impl<char, short, int> impl;
  if (impl.Direct() != 7) return 1;
  if (impl.ThroughThis() != 7) return 2;
  impl.Mark();
  if (impl.seen != 124) return 3;
  return 0;
}
