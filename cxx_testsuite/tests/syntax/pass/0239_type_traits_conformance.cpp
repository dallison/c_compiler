// RUN: -std=c++20
#include <functional>
#include <type_traits>

struct FiveArgCtor {
  int value;
  FiveArgCtor(int a, int b, int c, int d, int e) : value(a + b + c + d + e) {}
};

struct FiveArgCallable {
  int operator()(int a, int b, int c, int d, int e) const {
    return a + b + c + d + e;
  }
};

struct NoexceptCallable {
  int operator()(int value) noexcept { return value; }
};

struct ThrowingCallable {
  int operator()(int value) { return value; }
};

struct NoexceptCtor {
  NoexceptCtor() noexcept {}
};

struct ThrowingCtor {
  ThrowingCtor() {}
};

struct MemberData {
  int value;
};

struct MemberFn {
  int value;
  int add(int delta) const { return value + delta; }
};

static_assert(std::is_constructible<FiveArgCtor, int, int, int, int, int>::value,
              "arity-5 constructible");
static_assert(std::is_invocable<FiveArgCallable, int, int, int, int, int>::value,
              "arity-5 invocable");
static_assert(
    std::is_same<std::invoke_result_t<FiveArgCallable, int, int, int, int, int>,
                 int>::value,
    "arity-5 invoke_result_t");
static_assert(std::is_nothrow_constructible<NoexceptCtor>::value,
              "noexcept default constructor");
static_assert(!std::is_nothrow_constructible<ThrowingCtor>::value,
              "throwing default constructor");
static_assert(std::is_nothrow_invocable<NoexceptCallable, int>::value,
              "noexcept invocable");
static_assert(!std::is_nothrow_invocable<ThrowingCallable, int>::value,
              "throwing invocable");
static_assert(
    std::is_same<std::common_reference_t<const int&, volatile int&>,
                 const volatile int&>::value,
    "common_reference mixed cv lvalue refs");
static_assert(std::is_same<std::common_reference_t<int&&, const int&>,
                           const int&>::value,
              "common_reference rvalue with const lvalue");
static_assert(
    (std::is_same<std::common_reference_t<const int&, int&>, const int&>::value),
    "basic_common_reference path via common_reference");
static_assert(std::is_member_pointer<int (MemberData::*)>::value,
              "member pointer trait");
static_assert(std::is_member_object_pointer<int (MemberData::*)>::value,
              "member object pointer trait");

int main() {
  return 0;
}
