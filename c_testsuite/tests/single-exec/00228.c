/* _Pragma("...") operator (C99): equivalent to a #pragma directive but usable
   from macros and inline in the token stream.

   Drives #pragma pack via _Pragma both through a macro (push/pop) and directly,
   and uses a _Pragma carrying escaped quotes to exercise destringization.  The
   check is portable across targets: packing to 1 byte removes all padding, so a
   packed struct's size must equal the exact sum of its members regardless of
   the target's natural alignment or int size. */
#include <stdio.h>

#define BEGIN_PACK _Pragma("pack(push, 1)")
#define END_PACK   _Pragma("pack(pop)")

BEGIN_PACK
struct ViaMacro { char c; int i; };
END_PACK

_Pragma("pack(1)")
struct ViaDirect { char c; short s; int i; };
_Pragma("pack()")

/* Carries escaped quotes; must destringize to a valid diagnostic pragma and be
   accepted without disturbing parsing. */
#define IGNORE_UNUSED _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")
IGNORE_UNUSED

int main(void) {
  int unused_ok;
  int pass = 1;
  if (sizeof(struct ViaMacro) != 1 + sizeof(int)) pass = 0;
  if (sizeof(struct ViaDirect) != 1 + sizeof(short) + sizeof(int)) pass = 0;
  if (pass) {
    printf("PASS\n");
  } else {
    printf("FAIL\n");
  }
  return 0;
}
