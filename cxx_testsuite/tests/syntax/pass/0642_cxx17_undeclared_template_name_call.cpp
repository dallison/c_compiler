// RUN: -std=c++17
// `Undeclared<U>()` names a function template that argument-dependent lookup
// may find at instantiation; C++17 accepts it as an extension, like clang.

template <typename... T>
struct Alloc {
  template <typename U>
  int Off() const {
    return Undeclared<U>(U());
  }
};

int main() {
  return 0;
}
