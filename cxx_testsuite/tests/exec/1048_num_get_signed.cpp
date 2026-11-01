// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <limits>
#include <locale>
#include <sstream>

template <class T>
void read_number(const char* text, T& value, std::ios_base::iostate& err) {
  std::istringstream input(text);
  const std::num_get<char, std::istreambuf_iterator>& facet =
      std::use_facet<std::num_get<char, std::istreambuf_iterator>>(
          input.getloc());
  std::istreambuf_iterator in(input.rdbuf());
  std::istreambuf_iterator end;
  err = std::ios_base::goodbit;
  facet.get(in, end, input, err, value);
}

int main() {
  std::ios_base::iostate err = std::ios_base::goodbit;
  long long value = 7;
  read_number("-42", value, err);
  if (err != std::ios_base::goodbit || value != -42) return 1;

  value = 7;
  read_number("+15", value, err);
  if (err != std::ios_base::goodbit || value != 15) return 2;

  value = 7;
  read_number("-9223372036854775808", value, err);
  if (err != std::ios_base::goodbit ||
      value != std::numeric_limits<long long>::min()) {
    return 3;
  }

  value = 7;
  read_number("9223372036854775808", value, err);
  if ((err & std::ios_base::failbit) == 0 ||
      value != std::numeric_limits<long long>::max()) {
    return 4;
  }

  value = 7;
  read_number("-9223372036854775809", value, err);
  if ((err & std::ios_base::failbit) == 0 ||
      value != std::numeric_limits<long long>::min()) {
    return 5;
  }

  unsigned long long all = 1;
  read_number("18446744073709551615", all, err);
  if (err != std::ios_base::goodbit ||
      all != std::numeric_limits<unsigned long long>::max()) {
    return 6;
  }

  all = 1;
  read_number("18446744073709551616", all, err);
  if ((err & std::ios_base::failbit) == 0 ||
      all != std::numeric_limits<unsigned long long>::max()) {
    return 7;
  }

  all = 1;
  read_number("-1", all, err);
  if (err != std::ios_base::goodbit ||
      all != std::numeric_limits<unsigned long long>::max()) {
    return 8;
  }

  value = 7;
  read_number("abc", value, err);
  if ((err & std::ios_base::failbit) == 0 || value != 7) return 9;

  return 0;
}
