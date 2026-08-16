// RUN: -std=c++26

#include <tuple>
#include <utility>
#include <variant>

using info = decltype(^^int);

[[davecc::meta_intrinsic]] consteval bool is_void_type(info);
[[davecc::meta_intrinsic]] consteval bool is_integral_type(info);
[[davecc::meta_intrinsic]] consteval bool is_same_type(info, info);
[[davecc::meta_intrinsic]] consteval bool is_convertible_type(info, info);
[[davecc::meta_intrinsic]] consteval bool is_constructible_type(
    info, std::initializer_list<info>);
[[davecc::meta_intrinsic]] consteval bool is_invocable_type(
    info, std::initializer_list<info>);
[[davecc::meta_intrinsic]] consteval info remove_cvref(info);
[[davecc::meta_intrinsic]] consteval info add_lvalue_reference(info);
[[davecc::meta_intrinsic]] consteval info common_type(
    std::initializer_list<info>);
[[davecc::meta_intrinsic]] consteval info common_reference(
    std::initializer_list<info>);
[[davecc::meta_intrinsic]] consteval info invoke_result(
    info, std::initializer_list<info>);
[[davecc::meta_intrinsic]] consteval size_t tuple_size(info);
[[davecc::meta_intrinsic]] consteval info tuple_element(size_t, info);
[[davecc::meta_intrinsic]] consteval size_t rank(info);
[[davecc::meta_intrinsic]] consteval size_t extent(info, unsigned = 0);
[[davecc::meta_intrinsic]] consteval int type_order(info, info);

struct Callable {
  int operator()(int value) const { return value + 1; }
};

struct FiveArgCtor {
  FiveArgCtor(int, int, int, int, int) {}
};

static_assert(is_void_type(^^void));
static_assert(is_integral_type(^^int));
static_assert(!is_integral_type(^^float));
static_assert(is_same_type(^^int, ^^int));
static_assert(!is_same_type(^^int, ^^long));
static_assert(is_convertible_type(^^int, ^^long));
static_assert(is_constructible_type(^^FiveArgCtor,
                                    {^^int, ^^int, ^^int, ^^int, ^^int}));
static_assert(is_invocable_type(^^Callable, {^^int}));
static_assert(is_same_type(remove_cvref(^^const int&), ^^int));
static_assert(is_same_type(add_lvalue_reference(^^int), ^^int&));
static_assert(is_same_type(common_type({^^int, ^^long}), ^^long));
static_assert(is_same_type(common_reference({^^const int&, ^^volatile int&}),
                           ^^const volatile int&));
static_assert(is_same_type(invoke_result(^^Callable, {^^int}), ^^int));
static_assert(tuple_size(^^std::tuple<int, char>) == 2);
static_assert(is_same_type(tuple_element(1, ^^std::pair<int, char>), ^^char));
static_assert(rank(^^int[2][3]) == 2);
static_assert(extent(^^int[2][3], 0) == 2);
static_assert(extent(^^int[2][3], 1) == 3);
static_assert(type_order(^^int, ^^int) == 0);
static_assert(type_order(^^int, ^^long) != 0);

template <info T>
consteval bool dependent_trait() {
  return is_same_type(T, ^^int);
}

static_assert(dependent_trait<^^int>());

int main() {
  return 0;
}
