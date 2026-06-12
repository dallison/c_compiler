/* UTF-8 identifiers are accepted as implementation-defined C99 multibyte
   identifier characters.  This exercises lexer, preprocessor, semantic lookup,
   and target symbol emission. */
#include <stdio.h>

#define 増分(x) ((x) + 1)

static int café_global = 3;

struct boîte {
  int mañana;
};

static int mañana(int entrée) {
  int naïve = 増分(entrée);
  return naïve + café_global;
}

int main(void) {
  struct boîte boîte_locale;
  int δ = 10;
  boîte_locale.mañana = mañana(δ);
  printf("%d\n", boîte_locale.mañana);
  return 0;
}
