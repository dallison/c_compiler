// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <iomanip>
#include <sstream>
#include <string>

#if defined(__6502__) || defined(__wasm32__)

int main() {
  return 0;
}

#else

int main() {
  std::ostringstream formatting;
  formatting << std::setfill('*') << std::setw(5) << 42;
  if (formatting.str() != "***42") return 1;
  if (formatting.fill() != '*' || formatting.width() != 0) return 2;

  formatting.str("");
  formatting << std::setbase(16)
             << std::setiosflags(std::ios_base::showbase |
                                 std::ios_base::uppercase)
             << 255;
  if (formatting.str() != "0XFF") return 3;
  formatting << std::resetiosflags(std::ios_base::showbase |
                                   std::ios_base::uppercase)
             << std::setbase(10) << std::setprecision(3);
  if (formatting.precision() != 3) return 4;

  std::istringstream number("2a");
  int parsed = 0;
  number >> std::setbase(16) >> parsed;
  if (parsed != 42) return 5;

  std::ostringstream quoted_output;
  quoted_output << std::setfill('.') << std::setw(10)
                << std::quoted("a\"b\\c");
  if (quoted_output.str() != ".\"a\\\"b\\\\c\"") return 6;

  std::istringstream quoted_input("\"hello\\\" world\" tail");
  std::string text;
  quoted_input >> std::quoted(text);
  if (text != "hello\" world") return 7;
  quoted_input >> text;
  if (text != "tail") return 8;

  std::ostringstream money_output;
  std::string digits = "12345";
  money_output << std::put_money(digits);
  if (money_output.str() != "12345") return 9;
  std::istringstream money_input("6789");
  digits.clear();
  money_input >> std::get_money(digits);
  if (digits != "6789") return 10;

  struct tm date = {};
  date.tm_year = 124;
  date.tm_mon = 0;
  date.tm_mday = 2;
  std::ostringstream time_output;
  time_output << std::put_time(&date, "%Y-%m-%d");
  if (time_output.str() != "2024-01-02") return 11;

  struct tm read_date = {};
  std::istringstream time_input("2025-03-04");
  time_input >> std::get_time(&read_date, "%Y-%m-%d");
  if (time_input.fail() || read_date.tm_year != 125 ||
      read_date.tm_mon != 2 || read_date.tm_mday != 4) {
    return 12;
  }

  return 0;
}

#endif
