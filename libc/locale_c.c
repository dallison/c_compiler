//
//  locale_c.c
//  c_compiler
//

#include <limits.h>
#include <locale.h>
#include <string.h>

static char c_locale_name[] = "C";
static char decimal_point[] = ".";
static char empty[] = "";

static struct lconv c_locale = {
    decimal_point,
    empty,
    empty,
    empty,
    empty,
    empty,
    empty,
    empty,
    empty,
    empty,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
};

static int valid_category(int category) {
  return category >= LC_ALL && category <= LC_TIME;
}

char* setlocale(int category, const char* locale) {
  if (!valid_category(category)) {
    return 0;
  }
  if (locale == 0 || locale[0] == '\0' || strcmp(locale, "C") == 0 ||
      strcmp(locale, "POSIX") == 0) {
    return c_locale_name;
  }
  return 0;
}

struct lconv* localeconv(void) {
  return &c_locale;
}
