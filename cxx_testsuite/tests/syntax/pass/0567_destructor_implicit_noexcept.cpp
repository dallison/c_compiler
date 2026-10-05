// RUN: -std=c++20

// A destructor without a noexcept-specifier is non-throwing unless a base or
// member destructor is potentially-throwing ([except.spec]/8).
#include <type_traits>
struct Counted { int* d; constexpr ~Counted() { ++*d; } };
struct Throws { ~Throws() noexcept(false) {} };
struct HasThrows { Throws t; ~HasThrows() {} };
struct DerivesThrows : Throws { ~DerivesThrows() {} };
struct Explicit { ~Explicit() noexcept {} };
struct Defaulted { ~Defaulted() = default; };
struct OutOfLine { ~OutOfLine(); };
OutOfLine::~OutOfLine() {}
template <class T> struct Holder { T t; ~Holder() {} };
static_assert(std::is_nothrow_destructible_v<Counted>);
static_assert(!std::is_nothrow_destructible_v<Throws>);
static_assert(!std::is_nothrow_destructible_v<HasThrows>);
static_assert(!std::is_nothrow_destructible_v<DerivesThrows>);
static_assert(std::is_nothrow_destructible_v<Explicit>);
static_assert(std::is_nothrow_destructible_v<Defaulted>);
static_assert(std::is_nothrow_destructible_v<OutOfLine>);
static_assert(std::is_nothrow_destructible_v<Holder<int>>);
static_assert(!std::is_nothrow_destructible_v<Holder<Throws>>);
static_assert(noexcept(std::declval<Counted&>().~Counted()));
static_assert(!noexcept(std::declval<HasThrows&>().~HasThrows()));
int main() {}
