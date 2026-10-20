// RUN: -std=c++17
// EXPECT: No such symbol "undeclared_var"

template <typename T>
struct A {
  int Off() const;
};

template <typename T>
int A<T>::Off() const {
  return undeclared_var;
}

int main() {
  return 0;
}
