// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <regex>
#include <string>

int main() {
  std::regex identifier("[A-Za-z_]\\w*");
  if (!std::regex_match("value_42", identifier)) return 1;
  if (std::regex_match("42value", identifier)) return 2;

  std::string text = "prefix item-123 suffix";
  std::regex item("item-[0-9]+");
  std::smatch match;
  if (!std::regex_search(text, match, item)) return 3;
  if (!match.ready() || match.size() != 1) return 4;
  if (match.str() != "item-123" || match.position() != 7) return 5;
  if (match.prefix().str() != "prefix " ||
      match.suffix().str() != " suffix") {
    return 6;
  }

  std::regex insensitive("hello", std::regex::icase);
  if (!std::regex_match("HeLLo", insensitive)) return 7;

  std::string replaced =
      std::regex_replace(text, item, std::string("<$&>"));
  if (replaced != "prefix <item-123> suffix") return 8;

  std::regex anchored("^a.+z$");
  if (!std::regex_match("abcz", anchored)) return 9;
  if (std::regex_match("xabcz", anchored)) return 10;

  try {
    std::regex invalid("[abc");
    return 11;
  } catch (const std::regex_error& error) {
    if (error.code() != std::regex_constants::error_brack) return 12;
  }

  std::regex combined("[A-Z]+", std::regex::icase | std::regex::nosubs);
  if (!std::regex_match("AbC", combined)) return 13;
  if (combined.mark_count() != 0) return 14;

  try {
    std::regex bad_brace("a{");
    return 15;
  } catch (const std::regex_error& error) {
    if (error.code() != std::regex_constants::error_brace &&
        error.code() != std::regex_constants::error_badbrace) {
      return 16;
    }
  }

  std::smatch range_match;
  const std::string only = "item-123";
  if (!std::regex_match(only.begin(), only.end(), range_match, item)) {
    return 17;
  }
  if (range_match.str() != only) return 18;

  std::regex first_only("item");
  std::regex other("x");
  first_only.swap(other);
  if (!std::regex_search("x", first_only)) return 19;
  if (!std::regex_search("item", other)) return 20;

  return 0;
}
