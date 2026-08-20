// RUN: -std=c++29

#if __cpp_concepts != 202606L
#error "__cpp_concepts must advertise conditional noexcept requirements"
#endif

struct nothrow_callable {
  int operator()() noexcept;
};

struct throwing_callable {
  int operator()();
};

struct true_condition {
  constexpr explicit operator bool() const {
    return true;
  }
};

struct invalid_condition {};

template <typename T>
concept int_type = sizeof(T) == sizeof(int);

template <typename F, bool RequireNoexcept>
concept callable_as_requested = requires(F function) {
  { function() } noexcept(RequireNoexcept);
};

template <typename F, bool RequireNoexcept>
concept int_callable_as_requested = requires(F function) {
  { function() } noexcept(RequireNoexcept) -> int_type;
};

static_assert(callable_as_requested<nothrow_callable, true>);
static_assert(callable_as_requested<nothrow_callable, false>);
static_assert(!callable_as_requested<throwing_callable, true>);
static_assert(callable_as_requested<throwing_callable, false>);
static_assert(int_callable_as_requested<nothrow_callable, true>);
static_assert(int_callable_as_requested<throwing_callable, false>);

template <auto Condition>
concept condition_is_accepted = requires(nothrow_callable function) {
  { function() } noexcept(Condition);
};

static_assert(condition_is_accepted<true>);
static_assert(condition_is_accepted<false>);
static_assert(condition_is_accepted<1>);
static_assert(!condition_is_accepted<2>);

template <typename F>
concept class_condition_is_accepted = requires(F function) {
  { function() } noexcept(true_condition{});
};

static_assert(class_condition_is_accepted<nothrow_callable>);

template <typename F>
concept invalid_class_condition_is_rejected = requires(F function) {
  { function() } noexcept(invalid_condition{});
};

static_assert(!invalid_class_condition_is_rejected<nothrow_callable>);

int main(void) {
  return 0;
}
