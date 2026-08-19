// RUN: -std=c++29

constexpr auto raw_tokens = ^{ a + b };
constexpr auto token_interp = ^{ \(42) + x };
constexpr auto id_interp = ^{ \id("foo", 1) };
constexpr auto nested = ^{ \tokens(^{ inner ; }) };

constexpr auto boundary_left = ^{ a + };
constexpr auto boundary_right = ^{ + b };
constexpr auto boundary_both = ^{ \tokens(^{ x ; }) ; };

static_assert(raw_tokens == raw_tokens);
static_assert(token_interp == ^{ \(42) + x });
static_assert(id_interp == ^{ foo1 });
static_assert(nested == ^{ inner ; });
static_assert(boundary_left != boundary_right);
static_assert(boundary_both == ^{ x ; ; });

template <class T>
consteval auto dependent_tokens() {
  return ^{ T value[\(sizeof(T))]; };
}

static_assert(dependent_tokens<int>() == ^{ T value[\(sizeof(int))]; });

void token_sequence_semantics_runtime_discard() {
  (void)^{ \(1) \id("x") };
}
