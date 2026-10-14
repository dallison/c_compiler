// RUN: -std=c++17
// EXPECT_EXIT: 0

// `u.operator Box<T>()` names a conversion function whose type depends on a
// template parameter, so the member is looked up only after substitution.
// Detection idioms built on it (absl::StatusOr's
// HasConversionOperatorToStatusOr) must reject types without that operator,
// including when the failure is in an explicitly-specified callee's parameter
// type, an array bound, or a partial specialization's decltype argument.

#include <type_traits>
#include <utility>

template <class T> struct Box { T v; };

struct Conv {
  operator Box<int>() const { return {7}; }
};
struct Plain {};
struct HasFoo {
  int foo() const { return 1; }
};
struct NoFoo {};

template <class T, class U> T Unbox(U u) { return u.operator Box<T>().v; }

template <class T, class U>
auto Direct(int) -> decltype(std::declval<U>().operator Box<T>(),
                             std::true_type{});
template <class T, class U> std::false_type Direct(...);

template <class T, class U, class = void>
struct HasConversion : std::false_type {};
template <class T, class U>
void ConversionTest(char (*)[sizeof(std::declval<U>().operator Box<T>())]);
template <class T, class U>
struct HasConversion<T, U, decltype(ConversionTest<T, U>(0))>
    : std::true_type {};

template <class U> void ProbeParam(decltype(std::declval<U>().foo())*);
template <class U, class = void> struct ParamProbe : std::false_type {};
template <class U>
struct ParamProbe<U, decltype(ProbeParam<U>(nullptr))> : std::true_type {};

template <class U> void ProbeBound(char (*)[sizeof(std::declval<U>().foo())]);
template <class U, class = void> struct BoundProbe : std::false_type {};
template <class U>
struct BoundProbe<U, decltype(ProbeBound<U>(nullptr))> : std::true_type {};

static_assert(decltype(Direct<int, Conv>(0))::value, "");
static_assert(!decltype(Direct<int, Plain>(0))::value, "");
static_assert(HasConversion<int, Conv>::value, "");
static_assert(!HasConversion<int, Plain>::value, "");
static_assert(!HasConversion<long, Conv>::value, "");
static_assert(ParamProbe<HasFoo>::value && !ParamProbe<NoFoo>::value, "");
static_assert(BoundProbe<HasFoo>::value && !BoundProbe<NoFoo>::value, "");

int main() { return Unbox<int>(Conv{}) == 7 ? 0 : 1; }
