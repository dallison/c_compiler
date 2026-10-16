// RUN: -std=c++17
// EXPECT_EXIT: 0

// [meta.trans.other], [meta.rel]: invoke_result, is_invocable and friends --
// and C++17's deprecated result_of, which is SFINAE-friendly -- come from
// <type_traits> alone.
#include <type_traits>

static_assert(__cpp_lib_is_invocable >= 201703L);
static_assert(__cpp_lib_result_of_sfinae >= 201210L);

struct Callable {
  int operator()(int) const;
  double operator()(double, double) &&;
};

struct Thrower {
  void operator()() noexcept(false);
};

struct Data {
  long field;
  char method(int) const;
};

template <class F, class = void>
struct HasResult : std::false_type {};
template <class F>
struct HasResult<F, std::void_t<typename std::result_of<F>::type>>
    : std::true_type {};

static_assert(std::is_same_v<std::invoke_result_t<Callable, int>, int>);
static_assert(std::is_same_v<std::invoke_result_t<Callable, double, double>,
                             double>);
static_assert(!std::is_invocable_v<Callable&, double, double>);
static_assert(std::is_invocable_v<Callable, int>);
static_assert(std::is_invocable_r_v<long, Callable, int>);
static_assert(!std::is_invocable_r_v<int*, Callable, int>);
static_assert(std::is_invocable_r_v<void, Callable, int>);
static_assert(std::is_nothrow_invocable_v<void (*)() noexcept>);
static_assert(!std::is_nothrow_invocable_v<Thrower>);
static_assert(std::is_same_v<std::invoke_result_t<long Data::*, Data&>, long&>);
static_assert(std::is_same_v<std::invoke_result_t<char (Data::*)(int) const,
                                                  const Data*, int>,
                             char>);

static_assert(std::is_same_v<std::result_of_t<Callable(int)>, int>);
static_assert(std::is_same_v<std::result_of_t<long Data::*(Data&&)>, long&&>);
static_assert(HasResult<Callable(int)>::value);
static_assert(!HasResult<Callable(int*)>::value);

template <class F, class... Args,
          class R = std::invoke_result_t<F, Args...>>
constexpr int Pick(int) { return 1; }
template <class F, class... Args>
constexpr int Pick(...) { return 2; }

static_assert(Pick<Callable, int>(0) == 1);
static_assert(Pick<Callable, char*>(0) == 2);

int main() { return 0; }
