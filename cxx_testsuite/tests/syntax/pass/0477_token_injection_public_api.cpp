// RUN: -std=c++29

#include <meta>
#include <version>

#if __davecc_p3294_token_injection != 202406L
#error "missing DaveCC P3294 token-injection probe"
#endif

constexpr auto reported = ^{ int \id("value", 7) = \(7); };
static_assert(reported == ^{ int value7 = \(7); });

consteval void report() {
  std::meta::__report_tokens(reported);
}

static_assert((report(), true));

int main() { return 0; }
