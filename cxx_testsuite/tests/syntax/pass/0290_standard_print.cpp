// RUN: -std=c++23

#include <print>

#if __cpp_lib_print != 202207L
#error incorrect __cpp_lib_print
#endif

void check_print_surface(std::FILE* file) {
  int value = 7;
  auto args = std::make_format_args(value);
  std::print("{}", value);
  std::println("value={}", value);
  std::println();
  std::print(file, "{}", value);
  std::println(file, "{}", value);
  std::println(file);
  std::vprint_nonunicode("{}", args);
  std::vprint_nonunicode(file, "{}", args);
}
