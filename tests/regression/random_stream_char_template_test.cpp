// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>
#include <sstream>
#include <string>

template <class CharT>
CharT converted_space() {
  return CharT(' ');
}

int main() {
  if (converted_space<char>() != ' ') return 1;

  std::ranlux24_base base_source(17);
  base_source.discard(9);
  std::stringstream base_stream;
  base_stream << base_source;
  std::string base_text = base_stream.str();
  int base_spaces = 0;
  for (char value : base_text) {
    if (value == ' ') ++base_spaces;
  }
  if (base_spaces != 25) return 30 + base_spaces;
  std::ranlux24_base base_restored;
  base_stream >> base_restored;
  if (!base_stream) return 2;
  if (base_source != base_restored) return 3;

  std::ranlux24 source(17);
  source.discard(9);
  std::stringstream stream;
  stream << source;
  std::string text = stream.str();
  int spaces = 0;
  int zeros = 0;
  for (char value : text) {
    if (value == ' ') ++spaces;
    if (value == '\0') ++zeros;
  }
  if (zeros != 0) return 4;
  if (spaces != 26) return 5;

  std::ranlux24 restored;
  stream >> restored;
  if (!stream) return 6;
  if (source.base() != restored.base()) return 7;
  if (source != restored) return 8;
  return source() == restored() ? 0 : 9;
}
