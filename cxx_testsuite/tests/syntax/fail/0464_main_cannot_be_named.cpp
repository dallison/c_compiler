// RUN: -std=c++26
// EXPECT: the function 'main' cannot be named by an expression

int main() {
  return main();
}
