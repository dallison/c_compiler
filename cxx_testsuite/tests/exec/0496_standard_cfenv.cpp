// RUN: -std=c++11
// EXPECT_EXIT: 0

#include <cfenv>

int main() {
  if (std::fesetenv(FE_DFL_ENV) != 0)
    return 1;
  if (std::fegetround() != FE_TONEAREST)
    return 2;
  if (std::fesetround(FE_DOWNWARD) != 0 ||
      std::fegetround() != FE_DOWNWARD)
    return 3;
  if (std::feraiseexcept(FE_INVALID | FE_INEXACT) != 0)
    return 4;
  if (std::fetestexcept(FE_ALL_EXCEPT) != (FE_INVALID | FE_INEXACT))
    return 5;

  std::fexcept_t flags = 0;
  if (std::fegetexceptflag(&flags, FE_INVALID) != 0 ||
      flags != FE_INVALID)
    return 6;
  if (std::feclearexcept(FE_INVALID) != 0 ||
      std::fetestexcept(FE_ALL_EXCEPT) != FE_INEXACT)
    return 7;
  if (std::fesetexceptflag(&flags, FE_INVALID) != 0 ||
      std::fetestexcept(FE_ALL_EXCEPT) != (FE_INVALID | FE_INEXACT))
    return 8;

  std::fenv_t saved;
  if (std::feholdexcept(&saved) != 0 ||
      std::fetestexcept(FE_ALL_EXCEPT) != 0)
    return 9;
  if (std::feraiseexcept(FE_OVERFLOW) != 0 ||
      std::feupdateenv(&saved) != 0)
    return 10;
  if (std::fegetround() != FE_DOWNWARD ||
      std::fetestexcept(FE_ALL_EXCEPT) !=
          (FE_INVALID | FE_OVERFLOW | FE_INEXACT))
    return 11;
  if (std::fesetround(-1) == 0)
    return 12;
  return std::fesetenv(FE_DFL_ENV);
}
