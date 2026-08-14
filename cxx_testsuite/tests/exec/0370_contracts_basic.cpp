// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <contracts>

#if __cpp_contracts != 202606L
#error "__cpp_contracts has the wrong value"
#endif
#if __cpp_lib_contracts != 202502L
#error "__cpp_lib_contracts has the wrong value"
#endif
#if __cpp_lib_replaceable_contract_violation_handler != 202603L
#error "replaceable contract handler feature macro has the wrong value"
#endif

int twice(const int value)
    pre [[maybe_unused]] (value >= 0)
    post (result [[maybe_unused]]: result == value * 2) {
  return value * 2;
}

template <class T>
T add_one(const T value)
    pre (value >= 0)
    post (result: result == value + 1) {
  return value + 1;
}

auto deduced(const int value)
    post (result: result == value + 4) {
  return value + 4;
}

template <class T>
auto deduced_template(const T value)
    post (result: result == value + 5) {
  return value + 5;
}

int declared(const int value)
    pre (value > 0)
    post (result: result == value);

int declared(const int renamed) {
  return renamed;
}

struct value_holder {
  int value;

  int get() const
      pre (value >= 0)
      post (result: result == value) {
    return value;
  }
};

struct pair_value {
  int first;
  int second;
};

static const pair_value* returned_pair_address;

pair_value make_pair(const int value)
    post (result: (returned_pair_address = &result,
                   result.first == value &&
                       result.second == value + 1)) {
  return pair_value{value, value + 1};
}

const pair_value& identity(const pair_value& value)
    post (result: &result == &value) {
  return value;
}

int main() {
  contract_assert [[maybe_unused]] (twice(21) == 42);
  value_holder holder{11};
  pair_value pair = make_pair(7);
  auto lambda = [](const int value)
      pre (value >= 0)
      post (result: result == value * 3) {
    return value * 3;
  };
  return add_one(4) == 5 && deduced(3) == 7 &&
                 deduced_template(4) == 9 && declared(9) == 9 &&
                 holder.get() == 11 &&
                 pair.first == 7 && pair.second == 8 &&
                 returned_pair_address == &pair &&
                 &identity(pair) == &pair && lambda(6) == 18
             ? 0
             : 1;
}
