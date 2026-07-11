// RUN: -std=c++20
// EXPECT: 'inline namespace' cannot specify a nested-namespace-definition
inline namespace A::B {}

int main(void) {
  return 0;
}
