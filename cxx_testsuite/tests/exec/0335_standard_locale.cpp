// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <locale>
#include <sstream>
#include <string>
#include <ios>

#ifdef __cpp_exceptions
#define __LOCALE_UNSUPPORTED_TEST 1
#else
#define __LOCALE_UNSUPPORTED_TEST 0
#endif

static int callback_count = 0;

class custom_numpunct : public std::numpunct<char> {
 protected:
  char do_decimal_point() const override { return ','; }
  char do_thousands_sep() const override { return '.'; }
  std::string do_grouping() const override { return "\3"; }
  std::string do_truename() const override { return "yes"; }
  std::string do_falsename() const override { return "no"; }
};

void on_imbue(std::ios_base::event, std::ios_base&, int index) {
  if (index == 7) {
    ++callback_count;
  }
}

int main() {
  const std::locale classic = std::locale::classic();
  if (classic.name() != "C") {
    return 1;
  }
  const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(classic);
  if (!ctype.is(std::ctype_base::digit, '5') ||
      !ctype.is(std::ctype_base::xdigit, 'f') ||
      ctype.toupper('b') != 'B' || ctype.tolower('Z') != 'z') {
    return 2;
  }
  const std::ctype<wchar_t>& wctype =
      std::use_facet<std::ctype<wchar_t>>(classic);
  if (!wctype.is(std::ctype_base::alpha, L'q') ||
      wctype.toupper(L'm') != L'M') {
    return 3;
  }
  const std::numpunct<char>& np =
      std::use_facet<std::numpunct<char>>(classic);
  if (np.truename() != "true" || np.falsename() != "false") {
    return 4;
  }
  std::ostringstream output;
  output.imbue(classic);
  if (output.getloc() != classic) {
    return 5;
  }
  output << std::boolalpha << true << ' ' << 42;
  if (output.str() != "true 42") {
    return 6;
  }

  std::istringstream input("false 17");
  input.imbue(classic);
  bool flag = true;
  int value = 0;
  input >> std::boolalpha >> flag >> value;
  if (!input || flag || value != 17) {
    return 7;
  }

  std::stringstream buffer;
  std::locale old = buffer.rdbuf()->pubimbue(classic);
  if (buffer.rdbuf()->getloc() != classic) {
    return 8;
  }
  buffer.rdbuf()->pubimbue(old);

  std::ios_base::register_callback(on_imbue, 7);
  std::locale previous = std::locale::global(classic);
  if (previous.name() != "C" || callback_count != 0) {
    return 9;
  }
  (void)std::locale::global(previous);

#if __LOCALE_UNSUPPORTED_TEST
  bool threw = false;
  try {
    const char* bad_name = "en_US";
    (void)std::locale(bad_name);
  } catch (const std::runtime_error&) {
    threw = true;
  }
  if (!threw) {
    return 10;
  }
#endif

  if (!std::has_facet<std::num_get<char, std::istreambuf_iterator>>(
          classic)) {
    return 11;
  }

  std::locale customized(classic, new custom_numpunct);
  const std::numpunct<char>& custom =
      std::use_facet<std::numpunct<char>>(customized);
  if (custom.decimal_point() != ',' || custom.thousands_sep() != '.' ||
      custom.grouping() != "\3" || custom.truename() != "yes" ||
      custom.falsename() != "no" || customized.name() != "*") {
    return 12;
  }
  std::locale combined = classic.combine<std::numpunct<char>>(customized);
  if (std::use_facet<std::numpunct<char>>(combined).truename() != "yes") {
    return 13;
  }
  std::locale copied(classic, static_cast<custom_numpunct*>(nullptr));
  if (copied != classic) {
    return 14;
  }

  std::ostringstream grouped;
  grouped.imbue(customized);
  grouped << 1234567;
  if (grouped.str() != "1.234.567") {
    return 15;
  }
  grouped.str("");
  grouped << std::boolalpha << true;
  if (grouped.str() != "yes") {
    return 16;
  }

  return 0;
}
