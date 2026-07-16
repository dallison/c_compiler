// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Exercises the C++ "<c...>" wrappers for the standard C library headers.
// Each wrapper must (a) compile when included from C++, (b) expose the C
// names in namespace std, and (c) link against the C-linkage libc symbols.

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <csetjmp>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>

int cstddef_test() {
  std::size_t sz = sizeof(long);
  std::ptrdiff_t pd = (std::ptrdiff_t)-3;
  std::nullptr_t np = nullptr;
  int* p = (int*)np;
  if (sz != sizeof(void*)) return 1;
  if (pd != -3) return 2;
  if (p != nullptr) return 3;
  return 0;
}

int cstdlib_test() {
  if (std::abs(-7) != 7) return 1;
  if (std::atoi("123") != 123) return 2;
  if (std::strtol("2a", nullptr, 16) != 42) return 3;
  int* mem = (int*)std::malloc(sizeof(int) * 4);
  if (mem == nullptr) return 4;
  mem[0] = 11;
  mem[3] = 44;
  int ok = (mem[0] == 11 && mem[3] == 44) ? 0 : 5;
  std::free(mem);
  return ok;
}

int cstring_test() {
  const char* s = "hello";
  if (std::strlen(s) != 5) return 1;
  if (std::strcmp(s, "hello") != 0) return 2;
  char buf[8];
  std::memset(buf, 0, sizeof(buf));
  std::memcpy(buf, s, 5);
  if (std::strcmp(buf, "hello") != 0) return 3;
  if (std::strchr(buf, 'l') != buf + 2) return 4;
  return 0;
}

// Note: the printf/scanf family is variadic, and calling variadic functions
// currently misbehaves on the x86_64 interpreter (a pre-existing issue,
// unrelated to these header wrappers).  So this only checks that the names and
// types are visible in namespace std and resolve to the libc symbols at link
// time, without actually invoking a variadic call.
int cstdio_test() {
  std::FILE* fp = nullptr;
  std::fpos_t pos = (std::fpos_t)0;
  void* fns[] = {(void*)&std::snprintf, (void*)&std::fopen, (void*)&std::fclose,
                 (void*)&std::fread, (void*)&std::fwrite, (void*)&std::fseek};
  if (fp != nullptr) return 1;
  if (pos != 0) return 2;
  for (unsigned i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
    if (fns[i] == nullptr) return 3;
  }
  return 0;
}

int cmath_test() {
  if (std::sqrt(16.0) != 4.0) return 1;
  if (std::fabs(-2.5) != 2.5) return 2;
  return 0;
}

int cctype_test() {
  if (!std::isdigit('4')) return 1;
  if (std::isdigit('a')) return 2;
  if (!std::isalpha('z')) return 3;
  if (std::toupper('a') != 'A') return 4;
  if (std::tolower('Z') != 'z') return 5;
  return 0;
}

int climits_cerrno_test() {
  if (INT_MAX <= 0) return 1;
  if (EDOM == 0) return 2;
  return 0;
}

int ctime_types_test() {
  std::time_t t = (std::time_t)0;
  std::clock_t c = (std::clock_t)0;
  if (t != 0 || c != 0) return 1;
  return 0;
}

int cinttypes_test() {
  std::int32_t v = 32;
  // PRId32 must expand to a string literal, so "%" PRId32 is a single literal.
  const char* fmt = "%" PRId32;
  if (fmt[0] != '%') return 1;
  if (v != 32) return 2;
  return 0;
}

int cstdarg_test() {
  // Only checks that std::va_list is a usable type name; a real variadic call
  // is avoided (see the cstdio note above).
  std::va_list ap;
  (void)ap;
  return 0;
}

int csetjmp_test() {
  std::jmp_buf jb;
  volatile int reached = 0;
  int r = std::setjmp(jb);
  if (r == 0) {
    reached = 1;
    std::longjmp(jb, 42);
  }
  if (reached != 1) return 1;
  if (r != 42) return 2;
  return 0;
}

int cwchar_test() {
  std::size_t sz = sizeof(wchar_t);
  if (sz == 0) return 1;
  return 0;
}

int main(void) {
  int rc;
  if ((rc = cstddef_test()) != 0) return 10 + rc;
  if ((rc = cstdlib_test()) != 0) return 20 + rc;
  if ((rc = cstring_test()) != 0) return 30 + rc;
  if ((rc = cstdio_test()) != 0) return 40 + rc;
  if ((rc = cmath_test()) != 0) return 50 + rc;
  if ((rc = cctype_test()) != 0) return 60 + rc;
  if ((rc = climits_cerrno_test()) != 0) return 70 + rc;
  if ((rc = ctime_types_test()) != 0) return 80 + rc;
  if ((rc = cinttypes_test()) != 0) return 90 + rc;
  if ((rc = cstdarg_test()) != 0) return 100 + rc;
  if ((rc = csetjmp_test()) != 0) return 110 + rc;
  if ((rc = cwchar_test()) != 0) return 120 + rc;

  // cassert: assert() is a global macro (never std::assert).
  assert(1 == 1);
  return 0;
}
