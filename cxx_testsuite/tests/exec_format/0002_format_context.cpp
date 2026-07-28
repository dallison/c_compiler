// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <format>

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
