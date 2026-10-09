// RUN: -std=c++20
// Wide and UTF-16/32 string literals in constant evaluation: each element is
// one code unit of the literal's element type ([lex.string]), whether the
// literal is indexed directly, read through a pointer, passed to a function,
// or stored in a returned class.
constexpr const wchar_t* w = L"ab";
constexpr const char16_t* u16 = u"ab";

template <class C> constexpr unsigned long Len(const C* s) {
  unsigned long n = 0;
  while (s[n]) n++;
  return n;
}
template <class C> constexpr int Cmp(const C* a, const C* b, unsigned long n) {
  for (unsigned long i = 0; i < n; i++)
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  return 0;
}
template <class C> struct View {
  const C* p;
  unsigned long n;
};
template <class C> constexpr View<C> Make(const C* p) { return {p, Len(p)}; }
constexpr View<char> Literal() { return {"xyz", 3}; }

static_assert(L"ab"[1] == L'b' && w[1] == L'b' && w[2] == 0);
static_assert(u"ab"[1] == u'b' && u16[1] == u'b');
static_assert(U"ab"[1] == U'b');
static_assert(u"\u00e9"[0] == 0xe9 && L"\u00e9"[0] == 0xe9);
static_assert(U"\U0001F600"[0] == 0x1F600);
static_assert(u"\U0001F600"[0] == 0xD83D && u"\U0001F600"[1] == 0xDE00);
static_assert((u"\u00e9z"[1]) == u'z');
static_assert(sizeof(U"ab") == 12 && sizeof(u"ab") == 6);
static_assert(sizeof(L"ab") == 3 * sizeof(wchar_t));
static_assert(sizeof("a\0b") == 4 && "a\0b"[2] == 'b');

static_assert(Len(L"abc") == 3 && Len(w) == 2);
static_assert(Len(u"abc") == 3 && Len(u16) == 2 && Len(U"abcd") == 4);
static_assert(Len(u"\U0001F600") == 2);
static_assert(Cmp(L"ab", L"ac", 2) < 0 && Cmp(L"ab", L"ab", 2) == 0);
static_assert(Cmp(U"ab", U"aa", 2) > 0);

static_assert(Make("ab").p[1] == 'b' && *Make("ab").p == 'a');
static_assert(Make(L"ab").p[1] == L'b' && Make(L"ab").n == 2);
static_assert(Make(U"abc").p[2] == U'c');
static_assert(Literal().p[2] == 'z');
static_assert([] {
  const char* q = Make("ab").p;
  return q[1];
}() == 'b');

int main() { return 0; }
