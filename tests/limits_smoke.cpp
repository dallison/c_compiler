//
//  Return-code smoke test for <limits.h> UINT_MAX and <limits> numeric_limits.
//

#include <limits>
#include <climits>
#include <float.h>

static int fp_same(double a, double b) {
  return a == b ? 0 : 1;
}

int main() {
#if defined(__6502__)
  if (UINT_MAX != 65535U) return 1;
#endif

  if (!std::numeric_limits<int>::is_specialized) return 10;
  if (std::numeric_limits<int>::max() != INT_MAX) return 11;
  if (std::numeric_limits<int>::min() != INT_MIN) return 12;
#if defined(__6502__)
  if (std::numeric_limits<int>::digits10 != 4) return 13;
#else
  if (std::numeric_limits<int>::digits10 != 9) return 13;
#endif
  if (std::numeric_limits<int>::max_digits10 != 0) return 14;
  if (std::numeric_limits<int>::epsilon() != 0) return 15;
  if (std::numeric_limits<int>::radix != 2) return 16;

  if (!std::numeric_limits<unsigned int>::is_specialized) return 20;
  if (std::numeric_limits<unsigned int>::max() != UINT_MAX) return 21;

  if (!std::numeric_limits<char>::is_specialized) return 30;
#if CHAR_MIN < 0
  if (!std::numeric_limits<char>::is_signed) return 31;
#else
  if (std::numeric_limits<char>::is_signed) return 31;
#endif

  if (!std::numeric_limits<double>::is_specialized) return 40;
  if (std::numeric_limits<double>::radix != FLT_RADIX) return 41;
  if (std::numeric_limits<double>::digits != DBL_MANT_DIG) return 42;
  if (std::numeric_limits<double>::digits10 != DBL_DIG) return 43;
  if (std::numeric_limits<double>::max_digits10 !=
      2 + (DBL_MANT_DIG * 30103) / 100000) {
    return 44;
  }
  if (!std::numeric_limits<double>::has_infinity) return 45;
  if (!std::numeric_limits<double>::has_quiet_NaN) return 46;
  if (!std::numeric_limits<double>::is_iec559) return 47;
  if (std::numeric_limits<double>::has_denorm != std::denorm_present) return 48;
  if (fp_same(std::numeric_limits<double>::denorm_min(), DBL_TRUE_MIN) != 0) {
    return 49;
  }
  if (!(std::numeric_limits<double>::infinity() >
        std::numeric_limits<double>::max())) {
    return 50;
  }
  double quiet_nan = std::numeric_limits<double>::quiet_NaN();
  if (quiet_nan == quiet_nan) return 51;

  if (!std::numeric_limits<long long>::is_specialized) return 60;
  if (std::numeric_limits<long long>::digits != 63) return 61;

#if defined(__DAVECC__)
  if (std::numeric_limits<wchar_t>::max() != std::numeric_limits<int>::max()) {
    return 70;
  }
#endif

  if (FLT_TRUE_MIN <= 0.0F) return 80;
  if (DBL_TRUE_MIN <= 0.0) return 81;

  return 0;
}
