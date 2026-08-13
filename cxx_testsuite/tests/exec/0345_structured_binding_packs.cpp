// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <tuple>

struct triple {
  int first;
  int second;
  int third;
};

struct condition_pair {
  int first;
  int second;

  operator bool() const { return first != 0; }
};

template <class T>
int aggregate_pack(T value) {
  auto [...items [[maybe_unused]]] = value;
  static_assert(sizeof...(items) == 3);
  return (items + ...) + items...[1];
}

template <class T>
int middle_pack(T value) {
  auto [first, ...middle, last] = value;
  static_assert(sizeof...(middle) == 1);
  return first * 100 + middle...[0] * 10 + last;
}

template <class T>
int prefix_pack(T value) {
  auto [...prefix, last] = value;
  static_assert(sizeof...(prefix) == 2);
  return (prefix + ...) * 10 + last;
}

template <class T>
int suffix_pack(T value) {
  auto [first, ...suffix] = value;
  static_assert(sizeof...(suffix) == 2);
  return first * 100 + (suffix + ...);
}

template <class T>
int empty_pack(T value) {
  auto [first, second, third, ...rest] = value;
  static_assert(sizeof...(rest) == 0);
  return first + second + third;
}

template <class T>
int tuple_pack(T value) {
  auto [...items] = value;
  return items...[0] * 100 + items...[1] * 10 + items...[2];
}

template <class T>
int ordinary_dependent_binding(T value) {
  auto [first, second, third] = value;
  return first + second + third;
}

template <class Unused>
int known_pack_size() {
  auto [...items] = triple{2, 3, 4};
  static_assert(sizeof...(items) == 3);
  return (items + ...);
}

int sum_three(int first, int second, int third) {
  return first + second + third;
}

template <class T>
int expand_arguments(T value) {
  auto [...items] = value;
  return sum_three(items...);
}

template <class T>
int delayed_local_pack(T value) {
  auto copy = value;
  auto [first, ...rest] = copy;
  return first + (rest + ...);
}

template <class T, int N>
int array_pack(T (&value)[N]) {
  auto& [first, ...middle, last] = value;
  static_assert(sizeof...(middle) == 2);
  middle...[0] = 7;
  return first * 1000 + middle...[0] * 100 + middle...[1] * 10 + last;
}

template <class Range>
int range_pack(Range& values) {
  int total = 0;
  for (auto [first, ...items] : values) {
    total += first + (items + ...);
  }
  return total;
}

struct member_runner {
  template <class T>
  int run(T value) {
    auto [first, ...rest] = value;
    return first + (rest + ...);
  }
};

template <class T>
int dependent_condition(T value) {
  if (auto [first, second] = value) {
    return first + second;
  }
  return -1;
}

template <class T>
int delayed_dependent_condition(T value) {
  auto copy = value;
  if (auto [first, second] = copy) {
    return first + second;
  }
  return -1;
}

template <class T>
int pack_condition(T value) {
  if (auto [...items] = value) {
    return (items + ...);
  }
  return -1;
}

int main() {
  triple value{1, 2, 3};
  if (aggregate_pack(value) != 8) {
    return 1;
  }
  if (middle_pack(value) != 123) {
    return 2;
  }
  if (prefix_pack(value) != 33) {
    return 3;
  }
  if (suffix_pack(value) != 105) {
    return 4;
  }
  if (empty_pack(value) != 6) {
    return 5;
  }
  if (tuple_pack(std::tuple<int, char, long>(4, (char)5, 6L)) != 456) {
    return 6;
  }
  if (ordinary_dependent_binding(value) != 6) {
    return 7;
  }
  if (expand_arguments(value) != 6) {
    return 8;
  }
  if (delayed_local_pack(value) != 6) {
    return 9;
  }
  int array[4] = {1, 2, 3, 4};
  if (array_pack(array) != 1734 || array[1] != 7) {
    return 10;
  }
  triple values[2] = {{1, 2, 3}, {4, 5, 6}};
  if (range_pack(values) != 21) {
    return 11;
  }
  auto lambda = []<class T>(T item) {
    auto [...parts] = item;
    return (parts + ...);
  };
  if (lambda(value) != 6) {
    return 12;
  }
  member_runner runner;
  if (runner.run(value) != 6) {
    return 13;
  }
  if (known_pack_size<void>() != 9) {
    return 14;
  }
  if (auto [first, second] = condition_pair{1, 2}) {
    if (first + second != 3) {
      return 15;
    }
  } else {
    return 16;
  }
  if (auto [first, second] = condition_pair{0, 2}) {
    return 17;
  }
  if (dependent_condition(condition_pair{3, 4}) != 7 ||
      dependent_condition(condition_pair{0, 4}) != -1) {
    return 18;
  }
  if (delayed_dependent_condition(condition_pair{3, 4}) != 7 ||
      delayed_dependent_condition(condition_pair{0, 4}) != -1) {
    return 19;
  }
  if (pack_condition(condition_pair{3, 4}) != 7 ||
      pack_condition(condition_pair{0, 4}) != -1) {
    return 20;
  }
  int remaining = 2;
  int condition_total = 0;
  while (auto [first, second] = condition_pair{remaining--, 2}) {
    condition_total += first + second;
  }
  if (condition_total != 7) {
    return 21;
  }
  switch (auto [first, second] = condition_pair{1, 2}) {
    case true:
      if (first + second != 3) {
        return 22;
      }
      break;
    default:
      return 23;
  }
  return 0;
}
