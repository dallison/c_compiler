// RUN: -std=c++20
// EXPECT: Invalid type for member 'm' in instantiation of

// The member of a class nested in A<int> is instantiated with A<int>.
template <class T> struct A { struct B { decltype(T::x) m; }; };

int main() {
  A<int>::B b;
  return 0;
}
