// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <regex>
#include <string>

int main() {
  const std::string date = "2026-09-05";
  std::regex parts("([0-9]+)-([0-9]+)-([0-9]+)");
  std::smatch match;
  if (!std::regex_match(date, match, parts)) return 1;
  if (match.size() != 4) return 2;
  if (match[0].str() != "2026-09-05") return 3;
  if (match[1].str() != "2026" || match.position(1) != 0) return 4;
  if (match[2].str() != "09" || match.position(2) != 5) return 5;
  if (match[3].str() != "05" || match.position(3) != 8) return 6;

  std::cmatch cmatch;
  if (!std::regex_search("id=42;", cmatch, std::regex("id=([0-9]+)"))) {
    return 7;
  }
  if (cmatch.size() != 2 || cmatch[1].str() != "42") return 8;

  std::regex nested("a((b)c)");
  std::smatch nested_match;
  const std::string abc = "abc";
  if (!std::regex_match(abc, nested_match, nested)) return 9;
  if (nested_match.size() != 3) return 10;
  if (nested_match[1].str() != "bc" || nested_match[2].str() != "b") return 11;

  std::regex nosubs("([0-9]+)", std::regex::nosubs);
  if (nosubs.mark_count() != 0) return 12;
  std::smatch nosubs_match;
  const std::string number = "12";
  if (!std::regex_match(number, nosubs_match, nosubs)) return 13;
  if (nosubs_match.size() != 1 || nosubs_match[0].str() != "12") return 14;

  const std::string swapped =
      std::regex_replace(date, parts, std::string("$3/$2/$1"));
  if (swapped != "05/09/2026") return 15;

  const std::string tagged =
      std::regex_replace(date, parts, std::string("<$&>"));
  if (tagged != "<2026-09-05>") return 16;

  std::regex digit("([0-9])");
  const std::string pair = "a1b2";
  const std::string all =
      std::regex_replace(pair, digit, std::string("($1)"));
  if (all != "a(1)b(2)") return 17;
  const std::string first_only = std::regex_replace(
      pair, digit, std::string("($1)"),
      std::regex_constants::format_first_only);
  if (first_only != "a(1)b2") return 18;

  const std::string csv = "red,green";
  std::regex word("([A-Za-z]+)");
  std::sregex_token_iterator token(csv.begin(), csv.end(), word, 1);
  std::sregex_token_iterator last;
  if (token == last || token->str() != "red") return 19;
  ++token;
  if (token == last || token->str() != "green") return 20;
  ++token;
  if (token != last) return 21;

  return 0;
}
