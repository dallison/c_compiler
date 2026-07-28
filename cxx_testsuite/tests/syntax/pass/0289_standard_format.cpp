// RUN: -std=c++20

#include <format>

#if __cpp_lib_format != 201907L
#error incorrect __cpp_lib_format
#endif

struct point {
  int x;
};

template <>
struct std::formatter<point, char> {
  constexpr auto parse(std::format_parse_context& context) {
    return context.begin();
  }

  auto format(const point& value, std::format_context& context) const {
    return std::format_to(context.out(), "point({})", value.x);
  }
};

void check_format_surface() {
  std::string first = std::format("{} {:#x} {:.2f}", 7, 31, 2.5);
  std::string second;
  std::format_to(std::back_inserter(second), "{:>5}", "x");
  auto result = std::format_to_n(second.begin(), 2, "{}", 42);
  size_t size = std::formatted_size("{}", point{3});
  auto store = std::make_format_args(size);
  std::string third = std::vformat("{}", store);
  (void)first;
  (void)result;
  (void)third;
}
