// RUN: -std=c++20
// EXPECT: Invalid type for member 'm' in this instantiation

// No f accepts an int, so A<int> cannot declare m.
struct X {};
void f(X);
template <class T> struct A { decltype(f(T())) m; };

int main() {
  A<int> a;
  return 0;
}
