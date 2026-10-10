// A program may replace individual allocation functions while the
// library still provides the remaining ones (and set_new_handler).
#include <new>
#include <stdlib.h>

static int news = 0;
static int deletes = 0;

void* operator new(size_t n) {
  ++news;
  return malloc(n ? n : 1);
}

void operator delete(void* p) noexcept {
  ++deletes;
  free(p);
}

int main() {
  std::set_new_handler(nullptr);
  int* p = new int(3);
  if (news != 1 || *p != 3) return 1;
  delete p;
  if (deletes != 1) return 2;
  // The library's nothrow and sized forms forward to the replacements.
  int* q = new (std::nothrow) int(4);
  if (q == nullptr || *q != 4 || news != 2) return 3;
  ::operator delete(q, sizeof(int));
  if (deletes != 2) return 4;
  // The library's array forms forward to the replaced scalar forms.
  int* a = new int[4];
  a[3] = 5;
  if (news != 3) return 5;
  delete[] a;
  if (deletes != 3) return 6;
  return 0;
}
