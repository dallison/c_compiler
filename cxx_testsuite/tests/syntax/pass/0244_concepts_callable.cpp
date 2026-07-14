// RUN: -std=c++20
#include <concepts>
#include <functional>
#include <type_traits>

struct Callable {
  int operator()(int value) const { return value + 1; }
};

struct FiveArgCallable {
  int operator()(int a, int b, int c, int d, int e) const {
    return a + b + c + d + e;
  }
};

struct BoolReturning {
  bool operator()(int value) const { return value != 0; }
};

struct IntReturning {
  int operator()(int value) const { return value; }
};

struct MemberData {
  int value;
};

struct MemberFn {
  int value;
  int add(int delta) const { return value + delta; }
};

static_assert(std::invocable<Callable, int>);
static_assert(std::invocable<FiveArgCallable, int, int, int, int, int>);
static_assert(!std::invocable<Callable, int, int>);

static_assert(std::regular_invocable<Callable, int>);

struct EqPred {
  bool operator()(int left, int right) const { return left == right; }
};

struct LtPred {
  bool operator()(int left, int right) const { return left < right; }
};

static_assert(std::predicate<BoolReturning, int>);
static_assert(std::predicate<EqPred, int, int>);
// invoke_result_t of an int-returning callable is int (which, like any integer,
// is convertible to bool -- so IntReturning is itself a valid predicate).
static_assert(std::is_same<std::invoke_result_t<IntReturning, int>, int>::value);

static_assert(std::relation<EqPred, int, long>);
static_assert(std::equivalence_relation<EqPred, int, long>);
static_assert(std::strict_weak_order<LtPred, int, long>);

static_assert(std::invocable<int (MemberFn::*)(int) const, MemberFn, int>);
static_assert(
    std::is_same<std::invoke_result_t<int (MemberFn::*)(int) const, MemberFn,
                                                      int>,
                 int>::value);
static_assert(std::invocable<int MemberData::*, MemberData&>);

int main() {
  Callable callable;
  if (std::invoke(callable, 3) != 4) {
    return 1;
  }
  MemberFn object{5};
  int (MemberFn::*method)(int) const = &MemberFn::add;
  if (std::invoke(method, object, 2) != 7) {
    return 2;
  }
  MemberData data{9};
  int MemberData::*field = &MemberData::value;
  if (std::invoke(field, data) != 9) {
    return 3;
  }
  return 0;
}
