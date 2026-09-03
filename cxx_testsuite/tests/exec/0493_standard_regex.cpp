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
  return 0;
}
