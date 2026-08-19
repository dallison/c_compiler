// RUN: -std=c++29 -fconstexpr-eval=ast

constexpr auto ast_tokens = ^{ \(8) + \id("m") };
static_assert(ast_tokens == ^{ \(8) + \id("m") });
