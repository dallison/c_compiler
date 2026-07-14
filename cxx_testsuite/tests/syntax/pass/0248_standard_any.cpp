// RUN: -std=c++20

#include <any>
#include <exception>
#include <initializer_list>
#include <type_traits>
#include <utility>

struct Copyable {
  Copyable(int) {}
  Copyable(const Copyable&) = default;
};

struct MoveOnly {
  MoveOnly() {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
};

struct FromList {
  FromList(std::initializer_list<int>, int) {}
};

std::any& any_lvalue();
const std::any& const_any_lvalue();
std::any&& any_rvalue();

static_assert(std::is_default_constructible<std::any>::value);
static_assert(std::is_copy_constructible<std::any>::value);
static_assert(std::is_nothrow_move_constructible<std::any>::value);
static_assert(std::is_constructible<std::any, Copyable>::value);
static_assert(!std::is_constructible<std::any, MoveOnly>::value);
static_assert(!std::is_constructible<int&, const int&>::value);
static_assert(std::is_constructible<
              std::any, std::in_place_type_t<Copyable>, int>::value);
static_assert(std::is_constructible<
              std::any, std::in_place_type_t<const Copyable>, int>::value);
static_assert(std::is_base_of<std::bad_cast, std::bad_any_cast>::value);
static_assert(std::is_base_of<std::exception, std::bad_any_cast>::value);
static_assert(std::is_same<
              decltype(std::any_cast<int>(
                  const_any_lvalue())),
              int>::value);
static_assert(std::is_same<
              decltype(std::any_cast<int&>(any_lvalue())),
              int&>::value);
static_assert(std::is_same<
              decltype(std::any_cast<const int&>(
                  const_any_lvalue())),
              const int&>::value);
static_assert(std::is_same<
              decltype(std::any_cast<int>(any_rvalue())),
              int>::value);
static_assert(std::is_same<decltype(std::any_cast<int>(
                               static_cast<std::any*>(nullptr))),
                           int*>::value);
static_assert(std::is_same<decltype(std::make_any<Copyable>(1)),
                           std::any>::value);
static_assert(std::is_same<
              decltype(any_lvalue().emplace<const Copyable>(1)),
              Copyable&>::value);

void standard_any_surface() {
  std::any value;
  value.emplace<Copyable>(1);
  value.emplace<FromList>({1, 2}, 3);
  std::any other(std::in_place_type<Copyable>, 2);
  value.swap(other);
  swap(value, other);
  value.reset();
}
