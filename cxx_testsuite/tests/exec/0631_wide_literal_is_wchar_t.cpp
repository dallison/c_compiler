// RUN: -std=c++20
// EXPECT_EXIT: 0

// `L"..."` has type `const wchar_t[N]` and `L'x'` has type `wchar_t`, so
// they bind to `wchar_t` array references and pick `wchar_t` overloads.
template <class A, class B>
struct same {
  static constexpr bool value = false;
};
template <class A>
struct same<A, A> {
  static constexpr bool value = true;
};

static_assert(same<decltype(L"ab"), const wchar_t(&)[3]>::value);
static_assert(same<decltype(L'x'), wchar_t>::value);

struct View {
  unsigned long size;
  template <unsigned long N>
  constexpr View(const wchar_t (&)[N]) : size(N - 1) {}
};

unsigned long length(const View& v) { return v.size; }

int kind(int) { return 1; }
int kind(wchar_t) { return 2; }

int main() {
  if (length(L"abcd") != 4) {
    return 1;
  }
  if (kind(L'x') != 2) {
    return 2;
  }
  const wchar_t* text = L"hi";
  if (text[0] != L'h' || text[1] != L'i' || text[2] != 0) {
    return 3;
  }
  return 0;
}
