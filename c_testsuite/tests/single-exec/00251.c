// A static initializer may hold the address of an object reached by selecting a
// member, subscripting or pointer arithmetic.  Each of those shifts the address
// by a fixed number of bytes, and the initializer has to carry that offset: with
// only the symbol recorded, every pointer below would hold the address of the
// start of its object instead.  Each check compares against the same address
// computed at run time, so a lost or mis-scaled offset fails the test.
#include <stdio.h>

int numbers[60];

struct inner { int a, b; };
struct outer { struct inner in; int tail; };
struct outer object;
struct inner objects[4];

int *element = &numbers[18];
int *decayed = numbers + 3;
int *commuted = 3 + numbers;
int *after = &numbers[3] + 1;
int *before = &numbers[3] - 1;
char *bytes = (char *)&numbers[2 * 8 + 2];
char *bytes_back = (char *)&numbers[2 * 8 + 2] - 8;
// The offset can also be negative, which is a separate case for any target that
// has to encode it in a relocation.
char *below = (char *)numbers - 8;
int *below_int = numbers - 2;

int *member = &object.in.b;
int *tail = &object.tail;
int *through_pointer = &((&object.in)->b);
int *through_deref = &(*&object.in).b;
int *in_array = &objects[2].b;

struct pair { int *first; int *second; };
struct pair pair = {&numbers[1], &objects[3].a};

int main(void) {
  if (element != numbers + 18) return 1;
  if (decayed != numbers + 3) return 2;
  if (commuted != numbers + 3) return 3;
  if (after != numbers + 4) return 4;
  if (before != numbers + 2) return 5;
  if (bytes != (char *)(numbers + 18)) return 6;
  if (bytes_back != (char *)(numbers + 18) - 8) return 7;
  if (below != (char *)numbers - 8) return 8;
  if (below_int != numbers - 2) return 9;

  if (member != &object.in.b) return 10;
  if (tail != &object.tail) return 11;
  if (through_pointer != &object.in.b) return 12;
  if (through_deref != &object.in.b) return 13;
  if (in_array != &objects[2].b) return 14;

  if (pair.first != &numbers[1]) return 15;
  if (pair.second != &objects[3].a) return 16;

  // The offsets must be distinct from the object's own address, which is what a
  // dropped offset would collapse them to.
  if (element == numbers) return 17;
  if (member == (int *)&object) return 18;

  // Writing through the computed pointer must reach the member it names.
  *member = 7;
  if (object.in.b != 7) return 19;
  *in_array = 9;
  if (objects[2].b != 9) return 20;

  puts("ok");
  return 0;
}
