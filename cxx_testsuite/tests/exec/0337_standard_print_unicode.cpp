// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <cstdio>
#include <format>
#include <print>
#include <string>

int main() {
  const char* path = "/tmp/davecc_print_0337.txt";
  std::FILE* file = std::fopen(path, "w");
  if (file == nullptr) {
    return 1;
  }

  std::print(file, "{} ", 12);
  int value = 34;
  auto arguments = std::make_format_args(value);
  std::vprint_unicode(file, "{}", arguments);
  std::println(file, " {}", std::string("\xc3\xa9"));
  std::println(file);
  std::fclose(file);

  file = std::fopen(path, "r");
  if (file == nullptr) {
    return 2;
  }
  char buffer[64];
  size_t size = std::fread(buffer, 1, sizeof(buffer), file);
  std::fclose(file);
  std::string result(buffer, size);
  if (result != "12 34 \xc3\xa9\n\n") {
    return 3;
  }
  return 0;
}
