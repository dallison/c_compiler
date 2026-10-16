// RUN: -std=c++20
// EXPECT_EXIT: 0

// [conv.qual]: a multi-level pointer may gain cv-qualifiers below the top
// level only when every level above the change is const.  Span-like wrappers
// rely on this through `is_convertible<U (*)[], T (*)[]>`.
#include <type_traits>

static_assert(std::is_convertible_v<char**, const char* const*>);
static_assert(std::is_convertible_v<char**, char* const*>);
static_assert(!std::is_convertible_v<char**, const char**>);
static_assert(std::is_convertible_v<int***, const int* const* const*>);
static_assert(std::is_convertible_v<int***, int* const* const*>);
static_assert(!std::is_convertible_v<int***, const int* const**>);
static_assert(!std::is_convertible_v<int***, const int** const*>);
static_assert(std::is_convertible_v<char**, volatile char* const volatile*>);
static_assert(std::is_convertible_v<char (*)[], const char (*)[]>);

template <typename T>
struct Span {
  T* data;
  int size;
  Span(T* d, int n) : data(d), size(n) {}
  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>>>
  Span(const Span<U>& other) : data(other.data), size(other.size) {}
};

static int Length(Span<const char> s) { return s.size; }

int main() {
  char a[] = "ab";
  char b[] = "cde";
  char* strings[2] = {a, b};
  char** pp = strings;
  const char* const* view = pp;
  if (view[1][2] != 'e') return 1;
  char* const* fixed = pp;
  if (fixed[0] != a) return 2;
  Span<char> s(b, 3);
  if (Length(s) != 3) return 3;
  return 0;
}
