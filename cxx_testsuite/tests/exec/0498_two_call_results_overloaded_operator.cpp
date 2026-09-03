// RUN: -std=c++20
// EXPECT_EXIT: 0

struct pair_value {
  double first;
  double second;
};

static pair_value left_value() {
  return pair_value{2.0, 3.0};
}

static pair_value right_value() {
  return pair_value{4.0, 5.0};
}

static pair_value operator*(pair_value left, const pair_value& right) {
  return pair_value{
      left.first * right.first - left.second * right.second,
      left.first * right.second + left.second * right.first};
}

static pair_value operator+(pair_value left, const pair_value& right) {
  return pair_value{left.first + right.first, left.second + right.second};
}

static double consume(const pair_value& value) {
  return value.first + value.second;
}

template <class T>
struct pair_template {
  T first;
  T second;
};

template <class T>
static pair_template<T> operator+(pair_template<T> left,
                                  const pair_template<T>& right) {
  return pair_template<T>{left.first + right.first,
                          left.second + right.second};
}

template <class T>
static T consume_template(const pair_template<T>& value) {
  return value.first + value.second;
}

struct reference_constructor {
  double first;
  double second;

  reference_constructor(const double& left, const double& right)
      : first(left), second(right) {}
};

static double first_scalar() {
  return 6.0;
}

static double second_scalar() {
  return 7.0;
}

static float float_scalar() {
  return 8.0f;
}

int main() {
  pair_value result = left_value() * right_value();
  if (result.first != -7.0 || result.second != 22.0) return 1;
  if (consume(left_value() + right_value()) != 14.0) return 2;
  pair_template<double> first{1.0, 2.0};
  pair_template<double> second{3.0, 4.0};
  if (consume_template(first + second) != 10.0) return 3;
  reference_constructor constructed(first_scalar(), second_scalar());
  if (constructed.first != 6.0 || constructed.second != 7.0) return 4;
  reference_constructor converted(float_scalar(), second_scalar());
  return converted.first == 8.0 && converted.second == 7.0 ? 0 : 5;
}
