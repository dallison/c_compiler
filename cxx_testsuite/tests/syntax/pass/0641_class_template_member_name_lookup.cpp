// RUN: -std=c++17
// Names in class template member bodies that must still resolve: members
// declared later, anonymous union members, and members named from a lambda.

template <typename T>
struct A {
  int Later() const { return later; }
  int Anonymous() const { return u; }
  int FromLambda() const {
    return [&] { return later; }();
  }
  int OutOfLine() const;
  int later = 2;
  union {
    int u;
  };
};

template <typename T>
int A<T>::OutOfLine() const {
  return later;
}

int main() {
  A<int> a;
  a.u = 3;
  return a.Later() + a.Anonymous() + a.FromLambda() + a.OutOfLine() == 9 ? 0
                                                                         : 1;
}
