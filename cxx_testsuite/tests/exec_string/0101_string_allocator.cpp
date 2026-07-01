// std::string allocator-awareness: the third template parameter, the
// allocator_type typedef, get_allocator(), the allocator-extended
// constructors, and the fully-spelled basic_string<char, traits, allocator>.
// Returns 0 on success; a nonzero code identifies the first failing check.
#include <memory>
#include <string>

using std::string;

static bool eq(const char* a, const char* b) {
  while (*a != '\0' && *b != '\0') {
    if (*a != *b) return false;
    ++a;
    ++b;
  }
  return *a == *b;
}

int main() {
  // allocator_type is the nested typedef and names std::allocator<char>.
  std::string::allocator_type a1;
  std::allocator<char> a2;
  if (!(a1 == a2)) return 1;

  // get_allocator() returns an allocator that compares equal (stateless).
  string hello("hello");
  if (!(hello.get_allocator() == a2)) return 2;
  if (hello.get_allocator() != a2) return 3;

  // The fully-spelled type is the same instantiation std::string aliases.
  std::basic_string<char, std::char_traits<char>, std::allocator<char> > full(
      "world");
  if (!eq(full.c_str(), "world")) return 4;
  if (full.size() != 5) return 5;

  // Allocator-extended constructors.
  std::allocator<char> alloc;

  string only_alloc(alloc);
  if (!only_alloc.empty()) return 6;

  string from_cstr("abc", alloc);
  if (!eq(from_cstr.c_str(), "abc")) return 7;

  string filled(4, 'q', alloc);
  if (!eq(filled.c_str(), "qqqq")) return 8;

  string copy_with_alloc(hello, alloc);
  if (!eq(copy_with_alloc.c_str(), "hello")) return 9;
  if (copy_with_alloc.data() == hello.data()) return 10;  // deep copy

  // The stored allocator must drive real heap growth: append past the small
  // buffer so __reserve_for allocates and later frees through the member.
  string grown;
  for (int i = 0; i < 100; ++i) {
    grown.push_back('x');
  }
  if (grown.size() != 100) return 11;
  for (int i = 0; i < 100; ++i) {
    if (grown[i] != 'x') return 12;
  }
  if (!(grown.get_allocator() == a2)) return 13;

  // Concatenation still resolves the allocator-aware non-member operator+.
  string joined = from_cstr + string("def");
  if (!eq(joined.c_str(), "abcdef")) return 14;

  return 0;
}
