// RUN: -std=c++17
// EXPECT_EXIT: 0

int classify(double value) {
  switch (1) {
    case 1:
      int exp;
      auto mantissa = static_cast<double>(value);
      (void)exp;
      return (int)mantissa;
  }
  return 0;
}

int main() { return classify(1.5) == 1 ? 0 : 1; }
