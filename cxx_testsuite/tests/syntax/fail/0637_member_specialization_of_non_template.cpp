// RUN: -std=c++20
// EXPECT: 'f' is not a template

struct S {
  static int f(int);
};

template <>
int S::f<int>() {
  return 1;
}

template <>
int S::f<char>() {
  return 2;
}

int main() {
  return 0;
}
