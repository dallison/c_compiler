// RUN: -std=c++20 -fconstexpr-eval=audit

// std::string_view in constant expressions: every comparison and search, a
// view returned by substr that points into the original literal, and views
// passed to and read inside constexpr functions.

#include <compare>
#include <string_view>

constexpr std::string_view a("abc");
constexpr std::string_view b("abd");
constexpr std::string_view e;

static_assert(e.empty() && e.data() == nullptr);
static_assert(a.compare(b) < 0 && b.compare(a) > 0 && a.compare(a) == 0);
static_assert(a.compare("abc") == 0 && a.compare(0, 2, "ab") == 0);
static_assert(a.compare(0, 2, std::string_view("ab")) == 0);
static_assert(a == std::string_view("abc") && a != b);
static_assert(a < b && b > a && a <= a && b >= a);
static_assert((a <=> b) < 0 && (a <=> a) == 0 && (b <=> a) > 0);
static_assert(a.starts_with("ab") && a.starts_with('a') && !a.starts_with("b"));
static_assert(a.ends_with("bc") && a.ends_with('c'));
static_assert(a.contains("bc") && a.contains('b') && !a.contains("ca"));
static_assert(a.find('c') == 2 && a.find("bc") == 1 && a.rfind('a') == 0);
static_assert(a.find_first_of("cb") == 1 && a.find_last_of('a') == 0);
static_assert(a.find_first_not_of('a') == 1 && a.find_last_not_of('c') == 1);
static_assert(e.compare(a) < 0);

constexpr std::string_view tail = a.substr(1);
static_assert(tail.size() == 2 && tail[0] == 'b' && tail == "bc");
static_assert(a.substr(1)[0] == 'b' && a.substr(0, 2)[1] == 'b');
static_assert(a.substr(0, 2).compare(std::string_view("ab")) == 0);

static_assert(std::char_traits<char>::compare(a.data(), "ab", 2) == 0);
static_assert(std::char_traits<char>::compare(a.substr(0, 2).data(), "ab",
                                              2) == 0);

constexpr bool less(std::string_view x, std::string_view y) {
  return x.compare(y) < 0;
}
static_assert(less(a, b) && !less(b, a));

constexpr std::size_t length(std::string_view v) { return v.size(); }
static_assert(length(tail) == 2 && length(a) == 3);

constexpr int compare_prefix() {
  std::string_view s = a.substr(0, 2);
  return s.compare(std::string_view("ab"));
}
static_assert(compare_prefix() == 0);

int main() { return 0; }
