// RUN: -std=c++20

#include <locale>
#include <sstream>
#include <type_traits>

static_assert(std::is_copy_constructible_v<std::locale>);
static_assert(std::is_copy_assignable_v<std::locale>);

int main() {
  const std::locale classic = std::locale::classic();
  if (classic.name() != "C") {
    return 1;
  }
  if (!std::has_facet<std::ctype<char>>(classic) ||
      !std::has_facet<std::numpunct<char>>(classic) ||
      !std::has_facet<std::num_put<char, std::ostreambuf_iterator>>(
          classic)) {
    return 2;
  }
  const std::ctype<char>& facet = std::use_facet<std::ctype<char>>(classic);
  if (!facet.is(std::ctype_base::alpha, 'a') ||
      facet.is(std::ctype_base::digit, 'a')) {
    return 3;
  }
  const std::numpunct<char>& np =
      std::use_facet<std::numpunct<char>>(classic);
  if (np.decimal_point() != '.' || np.thousands_sep() != ',') {
    return 4;
  }
  std::locale copy(classic);
  if (copy != classic) {
    return 5;
  }
  return 0;
}
