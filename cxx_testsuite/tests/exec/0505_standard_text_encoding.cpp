// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <concepts>
#include <functional>
#include <locale>
#include <ranges>
#include <string_view>
#include <text_encoding>
#include <type_traits>

#if __cpp_lib_text_encoding != 202306L
#error "unexpected __cpp_lib_text_encoding value"
#endif

using encoding = std::text_encoding;

static_assert(encoding::max_name_length == 63);
static_assert(static_cast<int>(encoding::ASCII) == 3);
static_assert(static_cast<int>(encoding::UTF8) == 106);
static_assert(static_cast<int>(encoding::CP50220) == 2260);
static_assert(std::is_trivially_copyable_v<encoding>);

static_assert(encoding("UTF-8") == encoding::UTF8);
static_assert(encoding("utf8") == encoding::UTF8);
static_assert(encoding("u.t.f-008") == encoding::UTF8);
static_assert(encoding("utf-80") != encoding::UTF8);
static_assert(encoding("ISO_8859-1:1987") == encoding::ISOLatin1);
static_assert(encoding("csISO60DanishNorwegian") ==
              encoding::ISO60DanishNorwegian);
static_assert(encoding("windows-1252") == encoding::windows1252);
static_assert(encoding::literal() == encoding::UTF8);

static_assert(
    std::ranges::random_access_range<encoding::aliases_view>);
static_assert(std::ranges::borrowed_range<encoding::aliases_view>);
static_assert(std::same_as<
              std::ranges::range_value_t<encoding::aliases_view>,
              const char*>);
static_assert(std::same_as<
              std::ranges::range_reference_t<encoding::aliases_view>,
              const char*>);

int main() {
  encoding unknown;
  if (unknown != encoding::unknown || unknown.name() != nullptr ||
      !unknown.aliases().empty()) {
    return 1;
  }

  encoding utf8(encoding::UTF8);
  if (utf8.mib() != encoding::UTF8 ||
      std::string_view(utf8.name()) != std::string_view("UTF-8")) {
    return 2;
  }

  auto aliases = utf8.aliases();
  if (aliases.empty() ||
      std::string_view(aliases.front()) != std::string_view("UTF-8") ||
      aliases.end() - aliases.begin() !=
          static_cast<std::ptrdiff_t>(aliases.size())) {
    return 3;
  }
  bool found_short_name = false;
  for (const char* alias : aliases) {
    if (std::string_view(alias) == std::string_view("csUTF8")) {
      found_short_name = true;
    }
  }
  if (!found_short_name) {
    return 4;
  }

  encoding custom("WTF-8");
  if (custom.mib() != encoding::other ||
      std::string_view(custom.name()) != std::string_view("WTF-8") ||
      custom != encoding("wtf8") || custom == encoding("WTF-80") ||
      !custom.aliases().empty()) {
    return 5;
  }

  if (!encoding::environment_is<encoding::UTF8>() ||
      encoding::environment() != encoding::UTF8 ||
      std::locale::classic().encoding() != encoding::UTF8) {
    return 6;
  }

  if (std::hash<encoding>{}(encoding("utf8")) !=
      std::hash<encoding>{}(encoding::UTF8)) {
    return 7;
  }
  return 0;
}
