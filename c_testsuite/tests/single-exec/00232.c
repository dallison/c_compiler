/* Stringization (#) of macro arguments containing string and char literals.

   Regression test: the # operator encoded the token length from the unescaped
   text but stored the escaped text, desynchronizing the token stream and
   hanging the preprocessor on any argument that needed escaping (e.g. one
   containing a " or \).  Also exercises the common DO_PRAGMA(_Pragma(#x))
   idiom, which relies on the same path. */
#include <stdio.h>

#define STR(x) #x
#define XSTR(x) STR(x)

#define DO_PRAGMA(x) _Pragma(#x)
#define REMIND(text) DO_PRAGMA(message(#text))

REMIND(stringization works)

int main(void) {
  printf("%s\n", STR(plain));
  printf("%s\n", STR("quoted"));
  printf("%s\n", STR(call("arg")));
  printf("%s\n", STR('c'));
  printf("%s\n", XSTR(STR("nested")));
  return 0;
}
