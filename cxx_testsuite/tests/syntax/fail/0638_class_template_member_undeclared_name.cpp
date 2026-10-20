// RUN: -std=c++17
// EXPECT: No such symbol "undeclared_var"
// A non-dependent name is looked up at the template definition, even when
// the class template is never instantiated.

template <typename T>
struct A {
  int Off() const { return undeclared_var; }
};

int main() {
  return 0;
}
