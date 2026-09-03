// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <cstdio>
#include <print>
#include <string>

int main() {
  const char* path = "/tmp/davecc_standard_print_0495.txt";
  std::FILE* file = std::fopen(path, "w");
  if (file == nullptr) return 1;

  std::print(file, "{}-", 12);
  std::println(file, "{}", 34);
  std::println(file);
  int value = 56;
  std::format_args arguments = std::make_format_args(value);
  std::vprint_nonunicode(file, "{}", arguments);
  std::vprint_unicode(file, "-{}", arguments);
  std::fclose(file);

  file = std::fopen(path, "r");
  if (file == nullptr) return 2;
  char buffer[32] = {};
  size_t count = std::fread(buffer, 1, sizeof(buffer), file);
  std::fclose(file);
  return std::string(buffer, count) == "12-34\n\n56-56" ? 0 : 3;
}
