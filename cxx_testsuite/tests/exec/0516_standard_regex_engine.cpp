// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <regex>
#include <string>

int main() {
  std::regex either("cat|dog");
  if (!std::regex_match("cat", either)) return 1;
  if (!std::regex_match("dog", either)) return 2;
  if (std::regex_match("car", either)) return 3;

  std::regex grouped("(ab)+");
  std::smatch groups;
  const std::string abs = "ababab";
  if (!std::regex_match(abs, groups, grouped)) return 4;
  if (groups.size() != 2 || groups[1].str() != "ab") return 5;
  if (std::regex_match("a", grouped)) return 6;

  std::regex braces("a{2,4}");
  if (!std::regex_match("aa", braces)) return 7;
  if (!std::regex_match("aaaa", braces)) return 8;
  if (std::regex_match("a", braces)) return 9;
  if (std::regex_match("aaaaa", braces)) return 10;

  std::regex backref("(a)(b)\\2\\1");
  if (!std::regex_match("abba", backref)) return 11;
  if (std::regex_match("abab", backref)) return 12;

  std::regex choice("(a|b)c");
  std::smatch picked;
  const std::string bc = "bc";
  if (!std::regex_match(bc, picked, choice)) return 13;
  if (picked.size() != 2 || picked[1].str() != "b") return 14;

  std::regex noncap("(?:ab)+c");
  if (noncap.mark_count() != 0) return 15;
  if (!std::regex_match("ababc", noncap)) return 16;

  std::regex anchored("^a|b");
  std::smatch found;
  const std::string xb = "xb";
  if (!std::regex_search(xb, found, anchored) || found.str() != "b") return 17;
  const std::string ax = "ax";
  if (!std::regex_search(ax, found, anchored) || found.str() != "a") return 18;

  std::regex basic("\\(ab\\)\\{2\\}", std::regex::basic);
  if (basic.mark_count() != 1) return 19;
  std::smatch basic_match;
  const std::string abab = "abab";
  if (!std::regex_match(abab, basic_match, basic)) return 20;
  if (basic_match[1].str() != "ab") return 21;

  std::regex extended("one|two", std::regex::extended);
  if (!std::regex_match("two", extended)) return 22;

  std::regex grep("foo\nbar", std::regex::grep);
  if (!std::regex_match("foo", grep)) return 23;
  if (!std::regex_match("bar", grep)) return 24;

  std::regex egrep("one|two\nthree", std::regex::egrep);
  if (!std::regex_match("three", egrep)) return 25;

  return 0;
}
