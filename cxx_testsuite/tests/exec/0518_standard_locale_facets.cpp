// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <codecvt>
#include <locale>
#include <sstream>
#include <string>

int main() {
  std::locale classic = std::locale::classic();
  if (!std::has_facet<std::moneypunct<char> >(classic) ||
      !std::has_facet<std::money_put<char, std::ostreambuf_iterator> >(
          classic) ||
      !std::has_facet<std::time_put<char, std::ostreambuf_iterator> >(classic) ||
      !std::has_facet<std::messages<char> >(classic)) {
    return 1;
  }

  const std::moneypunct<char>& money =
      std::use_facet<std::moneypunct<char> >(classic);
  if (money.decimal_point() != '.' || money.frac_digits() != 0 ||
      money.curr_symbol() != "" || money.negative_sign() != "-") {
    return 2;
  }

  std::ostringstream money_out;
  const std::money_put<char, std::ostreambuf_iterator>& put_money =
      std::use_facet<std::money_put<char, std::ostreambuf_iterator> >(classic);
  std::string digits("12345");
  put_money.put(std::ostreambuf_iterator(money_out.rdbuf()), money_out, ' ',
                digits);
  if (money_out.str() != "12345") {
    return 3;
  }

  std::istringstream money_in("6789");
  std::ios_base::iostate money_err = std::ios_base::goodbit;
  std::string parsed_digits;
  const std::money_get<char, std::istreambuf_iterator>& get_money =
      std::use_facet<std::money_get<char, std::istreambuf_iterator> >(classic);
  get_money.get(std::istreambuf_iterator(money_in.rdbuf()),
                std::istreambuf_iterator(), false, money_in, money_err,
                parsed_digits);
  if ((money_err & std::ios_base::failbit) != 0 || parsed_digits != "6789") {
    return 4;
  }

  struct tm date = {};
  date.tm_year = 124;
  date.tm_mon = 0;
  date.tm_mday = 2;
  date.tm_wday = 2;
  std::ostringstream time_out;
  const std::time_put<char, std::ostreambuf_iterator>& put_time =
      std::use_facet<std::time_put<char, std::ostreambuf_iterator> >(classic);
  const char format[] = "%Y-%m-%d %a";
  const char* format_end = format;
  while (*format_end != '\0') {
    ++format_end;
  }
  put_time.put(std::ostreambuf_iterator(time_out.rdbuf()), time_out, ' ', &date,
               format, format_end);
  if (time_out.str() != "2024-01-02 Tue") {
    return 5;
  }

  struct tm read_date = {};
  std::istringstream time_in("2025-03-04");
  std::ios_base::iostate time_err = std::ios_base::goodbit;
  const std::time_get<char, std::istreambuf_iterator>& get_time =
      std::use_facet<std::time_get<char, std::istreambuf_iterator> >(classic);
  get_time.get_date(std::istreambuf_iterator(time_in.rdbuf()),
                    std::istreambuf_iterator(), time_in, time_err, &read_date);
  if ((time_err & std::ios_base::failbit) != 0 || read_date.tm_year != 125 ||
      read_date.tm_mon != 2 || read_date.tm_mday != 4) {
    return 6;
  }

  const std::messages<char>& msgs =
      std::use_facet<std::messages<char> >(classic);
  std::messages_base::catalog catalog = msgs.open("C", classic);
  std::string fallback("hello");
  if (msgs.get(catalog, 1, 1, fallback) != "hello") {
    return 7;
  }
  msgs.close(catalog);

  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  std::string bytes = converter.to_bytes(U'A');
  if (bytes.size() != 1 || bytes[0] != 'A') {
    return 8;
  }
  char32_t euro = 0x20ac;
  bytes = converter.to_bytes(euro);
  if (bytes.size() != 3 ||
      static_cast<unsigned char>(bytes[0]) != 0xe2 ||
      static_cast<unsigned char>(bytes[1]) != 0x82 ||
      static_cast<unsigned char>(bytes[2]) != 0xac) {
    return 9;
  }
  std::u32string restored = converter.from_bytes(bytes);
  if (restored.size() != 1 || restored[0] != euro) {
    return 10;
  }

  if (get_time.date_order() != std::time_base::ymd) {
    return 11;
  }

  return 0;
}
