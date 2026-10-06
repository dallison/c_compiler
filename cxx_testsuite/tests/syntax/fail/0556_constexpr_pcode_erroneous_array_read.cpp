// RUN: -std=c++26 -fconstexpr-eval=pcode
// EXPECT: erroneous value read in constexpr pcode

// In C++26 an uninitialized automatic variable holds an erroneous value, and a
// constant expression still may not read it.
constexpr int read_unwritten_element() {
  int values[3];
  values[0] = 1;
  values[2] = 3;
  return values[0] + values[1];
}

static_assert(read_unwritten_element() == 4);
