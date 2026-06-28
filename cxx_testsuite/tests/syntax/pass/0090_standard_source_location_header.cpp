// RUN: -std=c++20
#include <source_location>

#if __cpp_lib_source_location != 201907L
#error "source_location feature-test macro is missing"
#endif

static_assert(std::source_location().line() == 0,
              "source_location default constructor is constexpr");
static_assert(std::source_location().column() == 0,
              "source_location default column is constexpr");

static_assert(std::source_location::current().line() != 0,
              "source_location::current is consteval");

int source_location_default_line_expression(void) {
  return std::source_location().line();
}

std::source_location source_location_default_arg(
    std::source_location loc = std::source_location::current()) {
  return loc;
}

int standard_source_location_header(void) {
  std::source_location loc = std::source_location::current();
  return loc.line() + loc.column() + loc.file_name()[0] +
         loc.function_name()[0];
}

