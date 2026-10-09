// RUN: -std=c++20
// std::string_view in constant evaluation: comparisons with anything
// convertible to the view ([string.view.comparison]), including <=>, and the
// ""sv literal operator ([string.view.literals]).
#include <string_view>

using namespace std::literals;

constexpr std::string_view a = "hello";
constexpr std::string_view b = "world";

static_assert(a.size() == 5);
static_assert(a.compare("hello") == 0);
static_assert(a.compare("help") < 0);
static_assert(a.compare(std::string_view("hell")) > 0);
static_assert(a.compare(0, 4, "hell") == 0);

static_assert(a == "hello" && "hello" == a);
static_assert(a != "world" && "world" != a);
static_assert(a < b && b > a && a <= a && a >= a && !(b <= a));
static_assert(a < "world" && "world" > a && "abc" < a && a >= "hello");
static_assert(a <= "hello" && "hellp" >= a);
static_assert((a <=> "hello") == 0);
static_assert((a <=> "world") < 0);
static_assert(("world" <=> a) > 0);
static_assert((a <=> b) < 0);

static_assert(a.find("ll") == 2);
static_assert(a.rfind('l') == 3);
static_assert(a.starts_with("he") && a.ends_with("lo"));
static_assert(a.starts_with('h') && a.ends_with('o'));
static_assert(a.substr(1, 3) == "ell");
static_assert(a.find_first_of("lo") == 2);
static_assert(a.find_last_not_of('o') == 3);

static_assert("abc"sv.size() == 3);
static_assert("a\0b"sv.size() == 3);
static_assert("abc"sv == "abc" && "abc"sv < "abd");
static_assert(operator""sv("xy", 2).size() == 2);
static_assert(std::string_view_literals::operator""sv("xy", 2) == "xy");

static_assert(L"ab"sv.size() == 2 && L"ab"sv == L"ab" && L"ab"sv < L"ac");
static_assert(u"ab"sv.size() == 2 && u"ab"sv == u"ab" && u"ab"sv > u"aa");
static_assert(U"ab"sv.size() == 2 && U"ab"sv == U"ab" && U"ab"sv != U"b");
static_assert(u8"ab"sv.size() == 2 && u8"ab"sv == u8"ab");
static_assert(std::wstring_view(L"abc").find(L'c') == 2);
static_assert(std::u16string_view(u"\u00e9t\u00e9").rfind(u'\u00e9') == 2);
static_assert((std::u32string_view(U"\U0001F600") <=> U"\U0001F601") < 0);

constexpr bool Eq(std::string_view x, std::string_view y) { return x == y; }
constexpr bool Less(std::string_view x, const char* y) { return x < y; }
constexpr std::size_t Len(std::string_view x) { return x.size(); }
static_assert(Eq("abc", "abc") && !Eq("abc", "abd"));
static_assert(Less("abc", "abd") && !Less("abd", "abc"));
static_assert(Len("abcd") == 4);

int main() { return 0; }
