// RUN: -std=c++26

namespace detail {
inline constexpr int secret = 42;
}

namespace alias = detail;

constexpr auto ns = ^^alias;
static_assert([:ns:]::secret == 42);
