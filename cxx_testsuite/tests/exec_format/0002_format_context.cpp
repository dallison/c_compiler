// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <format>
#include <locale>
#include <string>

struct point {
  int x;
  int y;
};

template <>
struct std::formatter<point, char> {
  constexpr auto parse(std::format_parse_context& context) {
    return context.begin();
  }

  auto format(const point& value, std::format_context& context) const {
    auto out = context.out();
    *out++ = '(';
    *out++ = static_cast<char>('0' + value.x);
    *out++ = ',';
    *out++ = ' ';
    *out++ = static_cast<char>('0' + value.y);
    *out++ = ')';
    return out;
  }
};

int main() {
  if (std::format("{:{}}", 7, 4) != "   7") return 1;
  if (std::format("{:.{}f}", 3.24, 1) != "3.2") return 2;

  std::string output;
  auto end = std::format_to(std::back_inserter(output), "{}-{}", 12, 34);
  (void)end;
  if (output != "12-34") return 3;

  char short_output[3] = {};
  auto result = std::format_to_n(short_output, 3, "{}", 12345);
  if (result.out != short_output + 3 || result.size != 5 ||
      short_output[0] != '1' || short_output[2] != '3') return 4;
  if (std::formatted_size("{} {}", 123, "xy") != 6) return 5;
  if (std::format("{}", point{2, 5}) != "(2, 5)") return 6;

  std::wstring wide_output;
  std::format_to(std::back_inserter(wide_output), L"{}-{}", 7, 9);
  if (wide_output != L"7-9") return 8;

  wchar_t short_wide[2] = {};
  auto wide_result = std::format_to_n(short_wide, 2, L"{}", 456);
  if (wide_result.out != short_wide + 2 || wide_result.size != 3 ||
      short_wide[0] != L'4' || short_wide[1] != L'5') {
    return 9;
  }

  int runtime_value = 42;
  std::string runtime_output;
  std::vformat_to(std::back_inserter(runtime_output), "{}",
                  std::make_format_args(runtime_value));
  if (runtime_output != "42") return 10;

  std::wstring wide_runtime_output;
  std::vformat_to(std::back_inserter(wide_runtime_output), L"{}",
                  std::make_wformat_args(runtime_value));
  if (wide_runtime_output != L"42") return 11;

  const std::locale classic = std::locale::classic();
  char localized[2] = {};
  auto localized_result =
      std::format_to_n(localized, 2, classic, "{}", 789);
  if (localized_result.out != localized + 2 || localized_result.size != 3 ||
      std::formatted_size(classic, "{}", 789) != 3) {
    return 12;
  }

#ifdef __cpp_exceptions
  bool caught = false;
  try {
    std::format_args args;
    (void)std::vformat("{", args);
  } catch (const std::format_error&) {
    caught = true;
  }
  if (!caught) return 7;
#endif
  return 0;
}
