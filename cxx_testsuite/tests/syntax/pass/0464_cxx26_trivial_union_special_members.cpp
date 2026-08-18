// RUN: -std=c++26

#include <type_traits>

#if __cpp_trivial_union != 202603L
#error "__cpp_trivial_union must be 202603L"
#endif

struct nontrivial {
  nontrivial();
  nontrivial(const nontrivial&);
  nontrivial& operator=(const nontrivial&);
  ~nontrivial();
};

union storage {
  nontrivial object;
  int integer;
};

union explicitly_defaulted {
  nontrivial object;
  int integer;

  explicitly_defaulted() = default;
};

union initialized {
  nontrivial object = nontrivial();
  int integer;
};

union trivially_initialized {
  int integer = 42;
  long other;
};

union all_const {
  const int integer;
  const long other;
};

union user_constructed {
  nontrivial object;
  int integer;

  user_constructed() {}
};

static_assert(std::is_default_constructible_v<storage>);
static_assert(std::is_trivially_default_constructible_v<storage>);
static_assert(std::is_destructible_v<storage>);
static_assert(std::is_trivially_destructible_v<storage>);
static_assert(!std::is_copy_constructible_v<storage>);
static_assert(!std::is_copy_assignable_v<storage>);

static_assert(std::is_trivially_default_constructible_v<explicitly_defaulted>);
static_assert(std::is_trivially_destructible_v<explicitly_defaulted>);

static_assert(std::is_default_constructible_v<trivially_initialized>);
static_assert(!std::is_trivially_default_constructible_v<trivially_initialized>);
static_assert(std::is_trivially_destructible_v<trivially_initialized>);
static_assert(!std::is_default_constructible_v<all_const>);
static_assert(!std::is_destructible_v<initialized>);
static_assert(!std::is_destructible_v<user_constructed>);

template <class T>
union templated_storage {
  T object;
  int integer;
};

static_assert(
    std::is_trivially_default_constructible_v<templated_storage<nontrivial>>);
static_assert(
    std::is_trivially_destructible_v<templated_storage<nontrivial>>);

struct anonymous_wrapper {
  union {
    int nested;
    int fallback;
  };
};

constexpr int use_anonymous_union() {
  anonymous_wrapper value;
  value.nested = 42;
  return value.nested;
}

static_assert(use_anonymous_union() == 42);
