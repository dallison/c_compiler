// RUN: -std=c++29 -fconstexpr-eval=pcode

constexpr auto pcode_tokens = ^{ \(7) + \id("n") };
static_assert(pcode_tokens == ^{ \(7) + \id("n") });
