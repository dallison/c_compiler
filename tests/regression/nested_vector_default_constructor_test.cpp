// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <vector>

struct outer {
  struct param_type {
    param_type() : values{0.0, 1.0}, weights{1.0} {}

    std::vector<double> values;
    std::vector<double> weights;
  };

  param_type params;
};

int main() {
  outer value;
  if (value.params.values.size() != 2) return 1;
  if (value.params.values[0] != 0.0) return 2;
  if (value.params.values[1] != 1.0) return 3;
  if (value.params.weights.size() != 1) return 4;
  return value.params.weights[0] == 1.0 ? 0 : 5;
}
