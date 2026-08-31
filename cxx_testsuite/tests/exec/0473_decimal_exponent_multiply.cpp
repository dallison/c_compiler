// RUN: -std=c++20
// EXPECT_EXIT: 0

struct parse_result {
  const char* ptr;
  int error;
};

__attribute__((noinline))
const char* parse_special(const char* first, const char*, bool& nan, bool& inf) {
  nan = false;
  inf = false;
  return first;
}

__attribute__((noinline))
bool is_infinite(float value) {
  return value > 3.0e38f || value < -3.0e38f;
}

// Reduced copy of the decimal exponent path in from_chars. The surrounding
// state makes the loop-carried exponent spill on AArch64.
__attribute__((noinline))
parse_result parse_scientific(const char* first, const char* last, float& value,
                              int format) {
  const char* current = first;
  bool negative = false;
  if (current != last && *current == '-') {
    negative = true;
    ++current;
  }
  bool nan = false;
  bool inf = false;
  const char* special = parse_special(current, last, nan, inf);
  if (nan || inf) return {special, 0};

  unsigned long long significand = 0;
  int stored = 0;
  int frac_digits = 0;
  int extra_integer = 0;
  bool saw_digit = false;
  bool saw_point = false;
  for (; current != last; ++current) {
    if (*current == '.' && !saw_point) {
      saw_point = true;
      continue;
    }
    if (*current < '0' || *current > '9') break;
    saw_digit = true;
    if (stored < 19 &&
        significand <= (18446744073709551615ull - 9ull) / 10ull) {
      significand = significand * 10ull + (unsigned)(*current - '0');
      ++stored;
      if (saw_point) ++frac_digits;
    } else if (!saw_point) {
      ++extra_integer;
    }
  }
  if (!saw_digit) return {first, 1};

  bool saw_exp = false;
  int exponent = 0;
  const char* before_exp = current;
  if (current != last && (*current == 'e' || *current == 'E')) {
    const char* exp_pos = current + 1;
    bool exp_negative = false;
    if (exp_pos != last && (*exp_pos == '+' || *exp_pos == '-')) {
      exp_negative = *exp_pos == '-';
      ++exp_pos;
    }
    if (exp_pos != last && *exp_pos >= '0' && *exp_pos <= '9') {
      if (format == 2) return {first, 1};
      saw_exp = true;
      current = exp_pos;
      for (; current != last; ++current) {
        if (*current < '0' || *current > '9') break;
        if (exponent < 10000000)
          exponent = exponent * 10 + (*current - '0');
      }
      if (exp_negative) exponent = -exponent;
    } else {
      current = before_exp;
    }
  }
  if (format == 1 && !saw_exp) return {first, 1};

  int magnitude = exponent - frac_digits + extra_integer;
  if (magnitude > 38) return {current, 2};

  double parsed = 0.0;
  const char* p = first;
  if (negative) ++p;
  for (; p != current && *p != '.' && *p != 'e' && *p != 'E'; ++p)
    parsed = parsed * 10.0 + (double)(*p - '0');
  if (p != current && *p == '.') {
    ++p;
    double denom = 10.0;
    for (; p != current && *p != 'e' && *p != 'E'; ++p) {
      parsed = parsed + (double)(*p - '0') / denom;
      denom *= 10.0;
    }
  }
  if (exponent > 0)
    for (int i = 0; i < exponent; ++i) parsed *= 10.0;
  else if (exponent < 0)
    for (int i = 0; i < -exponent; ++i) parsed /= 10.0;
  if (negative) parsed = -parsed;

  float converted = (float)parsed;
  if (is_infinite(converted)) {
    value = converted;
    return {current, 2};
  }
  value = converted;
  return {current, 0};
}

int main() {
  const char text[] = "1e+10";
  float value = 0;
  parse_result result = parse_scientific(text, text + 5, value, 3);
  if (result.error != 0 || result.ptr != text + 5) return 1;
  return value == 1e10f ? 0 : 2;
}
