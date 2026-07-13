// RUN: -std=c++20
#include <concepts>
#include <type_traits>

struct PublicBase {
  int value;
};

struct PublicDerived : PublicBase {
  int extra;
};

struct PrivateDerived : private PublicBase {
};

struct Left : PublicBase {};
struct Right : PublicBase {};
struct AmbiguousDerived : Left, Right {};

struct ExplicitOnly {
  explicit ExplicitOnly(int) {}
};

struct ImplicitInt {
  ImplicitInt(int) {}
};

static_assert(std::integral<int>);
static_assert(!std::integral<float>);
static_assert(std::signed_integral<int>);
static_assert(!std::signed_integral<unsigned int>);
static_assert(std::unsigned_integral<unsigned int>);
static_assert(!std::unsigned_integral<int>);
static_assert(std::floating_point<double>);
static_assert(!std::floating_point<int>);

static_assert(std::same_as<int, int>);
static_assert(!std::same_as<int, long>);
static_assert(std::same_as<const int, const int>);

static_assert(std::derived_from<PublicDerived, PublicBase>);
static_assert(!std::derived_from<PublicBase, PublicDerived>);
static_assert(!std::derived_from<PrivateDerived, PublicBase>);
static_assert(!std::derived_from<AmbiguousDerived, PublicBase>);
static_assert(std::is_base_of_v<PublicBase, PrivateDerived>);
static_assert(std::is_base_of_v<PublicBase, AmbiguousDerived>);

static_assert(std::convertible_to<int, long>);
static_assert(!std::convertible_to<ExplicitOnly, int>);
static_assert(std::convertible_to<int, ImplicitInt>);

static_assert(std::common_reference_with<int&, const int&>);
static_assert(std::same_as<std::common_reference_t<int&, const int&>,
                           const int&>);
static_assert(std::common_reference_with<int&&, const int&>);

static_assert((std::is_same<typename std::common_type<int, long>::type,
                            typename std::common_type<long, int>::type>::value));

int main() {
  return 0;
}
