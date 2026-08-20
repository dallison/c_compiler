// RUN: -std=c++29 -fexceptions
// EXPECT_EXIT: 0

#include <cstdio>
#include <format>
#include <print>
#include <string>
#include <system_error>

template <class T>
concept supports_debug_format =
    requires(T& formatter) { formatter.set_debug_format(); };

static_assert(
    supports_debug_format<std::formatter<std::error_code, char>>);
static_assert(
    supports_debug_format<std::formatter<std::error_code, wchar_t>>);

class custom_category final : public std::error_category {
 public:
  const char* name() const noexcept override { return "custom"; }
  std::string message(int value) const override {
    return value == -7 ? std::string("\xff", 1)
                       : std::string("\xe1\x80", 2);
  }
};

int main() {
  std::error_code generic =
      std::make_error_code(std::errc::no_such_file_or_directory);
  std::string generic_text = std::format("{}", generic);
  std::string expected_generic =
      std::string("generic:") + std::format("{}", generic.value());
  if (generic_text != expected_generic) {
    return 1;
  }
  if (std::format("{:s}", generic) != generic.message()) {
    return 2;
  }
  if (std::format("{:?}", generic) !=
      std::format("{:?}", generic_text)) {
    return 3;
  }
  std::string message = generic.message();
  if (std::format("{:?s}", generic) != std::format("{:?}", message)) {
    return 4;
  }
  int dynamic_width = 14;
  if (std::format("{:*^15}", generic).size() != 15 ||
      std::format("{0:>{1}}", generic, dynamic_width).size() !=
          14) {
    return 5;
  }

  custom_category category;
  std::error_code custom(-7, category);
  if (std::format("{}", custom) != "custom:-7") {
    return 6;
  }
  if (std::format("{:s}", custom) != "\xef\xbf\xbd") {
    return 7;
  }
  std::error_code truncated(-8, category);
  if (std::format("{:s}", truncated) != "\xef\xbf\xbd") {
    return 8;
  }
  if (std::format(L"{}", custom) != L"custom:-7") {
    return 9;
  }

  const char* path = "/tmp/davecc_error_code_format_0434.txt";
  std::FILE* file = std::fopen(path, "w");
  if (file == nullptr) {
    return 10;
  }
  std::print(file, "{}", custom);
  std::fclose(file);
  file = std::fopen(path, "r");
  if (file == nullptr) {
    return 11;
  }
  char buffer[32];
  size_t size = std::fread(buffer, 1, sizeof(buffer), file);
  std::fclose(file);
  return std::string(buffer, size) == "custom:-7" ? 0 : 12;
}
