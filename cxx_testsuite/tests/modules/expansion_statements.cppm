export module expansion_statements;

export template <int... Values>
constexpr int expansion_sum() {
  int sum = 0;
  template for (constexpr int value : {Values...}) {
    sum += value;
  }
  return sum;
}
