// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <array>
#include <tuple>
#include <utility>

struct Triple {
  int x;
  long y;
  int z;
};

Triple make_triple(void) {
  Triple value = {1, 2L, 3};
  return value;
}

int aggregate_const_binding(void) {
  const auto [x, y, z] = make_triple();
  return x + static_cast<int>(y) + z;
}

int aggregate_reference_binding(void) {
  Triple t = {4, 5L, 6};
  auto& [x, y, z] = t;
  x += 10;
  y += 20;
  z += 30;
  return t.x + static_cast<int>(t.y) + t.z;
}

int array_binding(void) {
  int values[3] = {7, 8, 9};
  auto& [a, b, c] = values;
  b += 10;
  return values[0] + values[1] + values[2] + a + b + c;
}

int tuple_binding(void) {
  std::tuple<int, long, int> value(10, 20L, 30);
  auto [a, b, c] = value;
  return a + static_cast<int>(b) + c;
}

int four_element_tuple_binding(void) {
  std::tuple<int, long, int, long> value(1, 2L, 3, 4L);
  auto [a, b, c, d] = value;
  return std::get<0>(value) + static_cast<int>(std::get<1>(value)) +
         std::get<2>(value) + static_cast<int>(std::get<3>(value)) +
         a + static_cast<int>(b) + c + static_cast<int>(d);
}

int four_element_reference_tuple(void) {
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;
  std::tuple<int&, int&, int&, int&> value(a, b, c, d);
  std::get<0>(value) += 10;
  std::get<1>(value) += 20;
  std::get<2>(value) += 30;
  std::get<3>(value) += 40;
  return a + b + c + d;
}

int pair_binding(void) {
  std::pair<int, long> value(11, 12L);
  auto [a, b] = value;
  return a + static_cast<int>(b);
}

int standard_array_binding(void) {
  std::array<int, 3> value = {{13, 14, 15}};
  const auto [a, b, c] = value;
  return a + b + c;
}

int standard_array_reference_binding(void) {
  std::array<int, 3> value = {{16, 17, 18}};
  auto& [a, b, c] = value;
  a += 10;
  b += 20;
  c += 30;
  return value[0] + value[1] + value[2];
}

int standard_array_get(void) {
  std::array<int, 3> value = {{19, 20, 21}};
  std::get<1>(value) += 5;
  return value[0] + value[1] + value[2];
}

int range_for_structured_binding(void) {
  std::pair<int, int> values[2] = {{1, 2}, {3, 4}};
  int result = 0;
  for (auto [left, right] : values) {
    result += left + right;
  }
  for (auto& [left, right] : values) {
    left += 10;
    right += 20;
  }
  return result + values[0].first + values[0].second +
         values[1].first + values[1].second;
}

int main(void) {
  if (aggregate_const_binding() != 6) {
    return 1;
  }
  if (aggregate_reference_binding() != 75) {
    return 2;
  }
  if (array_binding() != 68) {
    return 3;
  }
  if (tuple_binding() != 60) {
    return 4;
  }
  if (pair_binding() != 23) {
    return 5;
  }
  if (standard_array_binding() != 42) {
    return 6;
  }
  if (standard_array_reference_binding() != 111) {
    return 7;
  }
  if (standard_array_get() != 65) {
    return 8;
  }
  if (range_for_structured_binding() != 80) {
    return 9;
  }
  if (four_element_tuple_binding() != 20) {
    return 10;
  }
  if (four_element_reference_tuple() != 110) {
    return 11;
  }
  return 0;
}
