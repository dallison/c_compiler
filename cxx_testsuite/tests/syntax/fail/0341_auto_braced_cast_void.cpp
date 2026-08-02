// RUN: -std=c++23
// EXPECT: a braced auto cast cannot initialize void

void operation() {}

int main() {
  (void)auto{operation()};
  return 0;
}
