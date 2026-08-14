// RUN: -std=c++26
// EXPECT_EXIT: 0

constexpr auto int_info = ^^int;

static_assert(int_info == ^^int);
static_assert(int_info != ^^long);

using integer = int;
constexpr auto alias_info = ^^integer;
static_assert(alias_info != int_info);

int reflected_function() {
  return 9;
}

struct reflected_class {
  int member;
};

constexpr auto function_info = ^^reflected_function;
constexpr auto member_info = ^^reflected_class::member;

int main() {
  typename[:^^int:] value = 42;
  reflected_class object{7};
  int function_value = [:function_info:]();
  int member_value = object.[:member_info:];
  return (value != 42) + (function_value != 9) + (member_value != 7);
}
