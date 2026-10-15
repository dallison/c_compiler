// `L"..."` initializes a `wchar_t[]` ([dcl.init.string]); C++ `wchar_t` is a
// distinct type, not `int` (googletest-printers-test).
int main() {
  wchar_t p[] = L"Hi";
  if (sizeof(p) != 3 * sizeof(wchar_t)) return 1;
  if (p[0] != L'H' || p[1] != L'i' || p[2] != 0) return 2;
  wchar_t* q = static_cast<wchar_t*>(p);
  q[0] = L'h';
  if (p[0] != L'h') return 3;

  const wchar_t sized[4] = L"ab";
  if (sized[1] != L'b' || sized[2] != 0 || sized[3] != 0) return 4;

  static wchar_t names[][3] = {L"ab", L"c"};
  if (names[0][1] != L'b' || names[1][0] != L'c' || names[1][1] != 0) return 5;

  struct Holder {
    wchar_t text[3];
  } holder = {L"xy"};
  if (holder.text[1] != L'y') return 6;
  return 0;
}
