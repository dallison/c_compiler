// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <regex>
#include <string>
#include <vector>

int main() {
  const std::string text = "aa12bb34cc";
  std::regex digits("[0-9]+");
  std::sregex_iterator it(text.begin(), text.end(), digits);
  std::sregex_iterator last;
  if (it == last) return 1;
  if (it->str() != "12" || it->position() != 2) return 2;
  ++it;
  if (it == last || it->str() != "34" || it->position() != 6) return 3;
  ++it;
  if (it != last) return 4;

  int count = 0;
  for (std::sregex_iterator current(text.begin(), text.end(), digits);
       current != last; ++current) {
    count += 1;
  }
  if (count != 2) return 5;

  const std::string csv = "red,green,blue";
  std::regex comma(",");
  std::sregex_token_iterator token(csv.begin(), csv.end(), comma, -1);
  std::sregex_token_iterator token_last;
  if (token == token_last || token->str() != "red") return 6;
  ++token;
  if (token == token_last || token->str() != "green") return 7;
  ++token;
  if (token == token_last || token->str() != "blue") return 8;
  ++token;
  if (token != token_last) return 9;

  std::regex word("[A-Za-z]+");
  std::sregex_token_iterator words(csv.begin(), csv.end(), word, 0);
  if (words == token_last || words->str() != "red") return 10;
  ++words;
  if (words == token_last || words->str() != "green") return 11;
  ++words;
  if (words == token_last || words->str() != "blue") return 12;
  ++words;
  if (words != token_last) return 13;

  std::smatch found;
  if (!std::regex_search(text.begin(), text.end(), found, digits)) return 14;
  if (found.str() != "12") return 15;

  std::cregex_iterator c_it(text.c_str(), text.c_str() + text.size(), digits);
  std::cregex_iterator c_last;
  if (c_it == c_last || c_it->str() != "12") return 16;
  ++c_it;
  if (c_it == c_last || c_it->str() != "34") return 17;

  return 0;
}
