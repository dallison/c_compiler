// RUN: -std=c++20
// EXPECT_EXIT: 0
// A prefixed string literal keeps all of its code units, including the zero
// bytes inside wide code units, wherever it appears: in parentheses (which the
// parser may scan tentatively), as a sizeof operand, and in a constant.
int Read(const char16_t* p, int i) { return p[i]; }

int main() {
  if (sizeof(U"ab") != 12 || sizeof(u"ab") != 6) return 1;
  if (sizeof(L"ab") != 3 * sizeof(wchar_t)) return 2;
  const char16_t* q = (u"\u00e9z");
  if (q[0] != 0xe9 || q[1] != u'z' || q[2] != 0) return 3;
  const char32_t* p = (U"\U0001F600x");
  if (p[0] != 0x1F600 || p[1] != U'x' || p[2] != 0) return 4;
  const wchar_t* w = (L"\u00e9z");
  if (w[0] != 0xe9 || w[1] != L'z' || w[2] != 0) return 5;
  if ((int)(u"\u00e9"[0]) != 0xe9) return 6;
  if ((int)(U"\U0001F600"[0] >> 8) != 0x1F6) return 7;
  if (Read((u"\U0001F600"), 1) != 0xDE00) return 8;
  constexpr char16_t c = (u"\u00e9z")[1];
  if (c != u'z') return 9;
  const char* n = ("a\0b");
  if (n[2] != 'b' || sizeof("a\0b") != 4) return 10;
  return 0;
}
