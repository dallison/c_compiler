// RUN: -std=c++17
// EXPECT_EXIT: 0

#define F(x) (x)

int main() {
  int a = F(1 +
            // comment between tokens
            2);
  int b = F((
      // also inside parens
      1));
  return a + b - 4;
}
