// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <print>

int main() {
  std::FILE file = {};
  file.fd = 1;
  file.buffering_mode = _IONBF;

  std::print(&file, "{} ", 12);
  std::println(&file, "{}", "items");
  int value = 7;
  auto args = std::make_format_args(value);
  std::vprint_nonunicode(&file, "{}", args);
  std::println(&file);

  std::print("");
  return 0;
}
