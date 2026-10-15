// File-scope __int128 objects are statically initialized with both words.
#include <stdio.h>

__int128 shifted = (__int128)1 << 100;
unsigned __int128 all = ~(unsigned __int128)0;
__int128 negative = -2;
struct Holder {
  int tag;
  __int128 value;
};
struct Holder holder = {1, (__int128)3 << 64};
__int128 values[2] = {7, (__int128)1 << 64};

int main(void) {
  if ((shifted >> 100) != 1) return 1;
  if (all + 1 != 0 || (unsigned long long)(all >> 64) != ~0ull) return 2;
  if (negative != -2 || (negative >> 64) != -1) return 3;
  if (holder.tag != 1 || (holder.value >> 64) != 3) return 4;
  if (values[0] != 7 || (values[1] >> 64) != 1) return 5;
  printf("ok\n");
  return 0;
}
