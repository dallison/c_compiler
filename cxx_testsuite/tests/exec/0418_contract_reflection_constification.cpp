// RUN: -std=c++26
// EXPECT_EXIT: 0

int global_value = 4;

int checked(const int value, int* pointed)
    pre ([:^^value:] >= 0)
    pre ([:^^global_value:] == 4)
    pre ((++*pointed, true))
    post (result: result == [:^^value:] + [:^^global_value:]) {
  return value + global_value;
}

template <class T>
T checked_template(const T value)
    pre ([:^^value:] >= 0)
    post (result: result == [:^^value:] + 1) {
  return value + 1;
}

int checked_lambda(const int value)
    pre ([&] {
      int predicate_local = 0;
      ++predicate_local;
      return predicate_local == 1 && [:^^value:] >= 0;
    }()) {
  return value;
}

int main() {
  int pointed = 1;
  int local = 3;
  int pair[2] = {2, 3};
  auto& [first, second] = pair;
  contract_assert ([:^^local:] == 3);
  contract_assert ([:^^first:] + [:^^second:] == 5);
  return checked(5, &pointed) == 9 && pointed == 2 &&
                 checked_template(6) == 7 && checked_lambda(8) == 8
             ? 0
             : 1;
}
