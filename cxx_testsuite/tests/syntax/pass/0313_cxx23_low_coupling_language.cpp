// RUN: -std=c++23

template <class A, class B>
struct same_type {
  static constexpr bool value = false;
};

template <class A>
struct same_type<A, A> {
  static constexpr bool value = true;
};

static_assert(same_type<decltype(0uz), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0uZ), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0Uz), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0UZ), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0zu), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0zU), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0Zu), decltype(sizeof(0))>::value);
static_assert(same_type<decltype(0ZU), decltype(sizeof(0))>::value);
static_assert(0z - 1 < 0);
static_assert(0Z - 1 < 0);

static_assert(same_type<decltype(auto(1)), int>::value);
static_assert(auto{1} == 1);

constexpr auto static_lambda = [] static { return 40; };
constexpr auto static_lambda_with_parens = []() static { return 41; };
constexpr auto generic_static_lambda =
    []<class T>(T value) static { return value + 1; };

int use_static_lambdas() {
  return static_lambda() + static_lambda_with_parens() +
         generic_static_lambda(1);
}

struct move_only_exception {
  move_only_exception() = default;
  move_only_exception(const move_only_exception&) = delete;
  move_only_exception(move_only_exception&&) = default;
};

void throw_parameter(move_only_exception value) {
  throw value;
}

int test_alias_init_statements() {
  int sum = 0;
  if (using value_type = int; value_type value = 1) {
    sum += value;
  }
  switch (using value_type = int; value_type value = 2) {
    case 2:
      sum += value;
      break;
  }
  for (using value_type = int; sum < 4;) {
    value_type increment = 1;
    sum += increment;
  }
  int values[2] = {3, 4};
  for (using value_type = int; value_type value : values) {
    sum += value;
  }
  return sum;
}

int test_trailing_label(bool skip) {
  {
    if (skip) {
      goto done;
    }
    return 1;
done:
  }
  return 0;
}

void test_trailing_switch_labels(int value) {
  switch (value) {
    case 1:
  }
  switch (value) {
    default:
  }
}

int main() {
  return test_alias_init_statements() + test_trailing_label(true);
}
