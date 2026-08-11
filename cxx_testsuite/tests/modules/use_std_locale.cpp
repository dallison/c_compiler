import std;

int main() {
  const std::locale classic = std::locale::classic();
  if (classic.name() != "C") {
    return 1;
  }
  const std::ctype<char>& facet = std::use_facet<std::ctype<char>>(classic);
  if (!facet.is(std::ctype_base::digit, '9')) {
    return 2;
  }
  std::ostringstream output;
  output.imbue(classic);
  output << 12;
  if (output.str() != "12") {
    return 3;
  }
  return 0;
}
