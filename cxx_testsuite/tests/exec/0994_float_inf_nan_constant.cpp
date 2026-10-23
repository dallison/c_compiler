// RUN: -std=c++17
// EXPECT_EXIT: 0

const double kInfinity = 1.0 / 0.0;
const double kNaN = 0.0 / 0.0;

int main() {
  if (!(kInfinity > 0.0)) return 1;
  if (kNaN == kNaN) return 2;
  return 0;
}
