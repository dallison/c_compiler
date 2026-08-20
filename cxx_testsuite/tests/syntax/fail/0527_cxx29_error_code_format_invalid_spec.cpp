// RUN: -std=c++29 -fexceptions
// EXPECT: constant evaluation ended with an uncaught exception

#include <format>
#include <system_error>

consteval bool parse_invalid_error_code_format() {
  std::formatter<std::error_code, char> formatter;
  std::format_parse_context context("d", 1);
  formatter.parse(context);
  return true;
}

static_assert(parse_invalid_error_code_format());
