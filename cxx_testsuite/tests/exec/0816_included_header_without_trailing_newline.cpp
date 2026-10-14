// A header whose last line has no newline must not swallow the includer's
// next line, so the directive after the #include still takes effect.
#include "cxx_testsuite/include/no_trailing_newline.hpp"
#define AFTER_HEADER 5

int main() {
#ifndef AFTER_HEADER
  return 1;
#else
  return no_trailing_newline::Value() + AFTER_HEADER == 12 ? 0 : 2;
#endif
}
