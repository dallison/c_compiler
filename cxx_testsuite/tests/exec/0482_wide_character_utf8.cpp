// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <cuchar>
#include <cwchar>
#include <cwctype>
#include <ctime>

int main() {
  const char utf8[] = "\x41\xc2\xa2\xe2\x82\xac\xf0\x9f\x98\x80";
  const char* source = utf8;
  wchar_t wide[5];
  std::mbstate_t state = {};
  if (std::mbsrtowcs(wide, &source, 5, &state) != 4 || source != 0) return 1;
  if (wide[0] != L'A' || wide[1] != 0xa2 || wide[2] != 0x20ac ||
      wide[3] != 0x1f600 || wide[4] != 0) return 2;

  char encoded[16];
  const wchar_t* wide_source = wide;
  state = std::mbstate_t();
  if (std::wcsrtombs(encoded, &wide_source, sizeof(encoded), &state) != 10 ||
      wide_source != 0) return 3;
  for (int i = 0; i < 11; ++i) {
    if (encoded[i] != utf8[i]) return 4;
  }

  wchar_t partial = 0;
  state = std::mbstate_t();
  if (std::mbrtowc(&partial, "\xe2", 1, &state) != (std::size_t)-2) return 5;
  if (std::mbrtowc(&partial, "\x82\xac", 2, &state) != 2 ||
      partial != 0x20ac) return 6;

  char16_t first = 0;
  char16_t second = 0;
  state = std::mbstate_t();
  if (std::mbrtoc16(&first, "\xf0\x9f\x98\x80", 4, &state) != 4 ||
      first != 0xd83d) return 7;
  if (std::mbrtoc16(&second, "", 1, &state) != (std::size_t)-3 ||
      second != 0xde00) return 8;

  char32_t scalar = 0;
  state = std::mbstate_t();
  if (std::mbrtoc32(&scalar, "\xf0\x9f\x98\x80", 4, &state) != 4 ||
      scalar != 0x1f600) return 9;

  if (!std::iswalpha(L'A') || !std::iswdigit(L'7') ||
      std::towupper(L'b') != L'B') return 10;
  if (!std::iswctype(L' ', std::wctype("space")) ||
      std::towctrans(L'Q', std::wctrans("tolower")) != L'q') return 11;

  wchar_t text[16];
  std::wcscpy(text, L"wide");
  std::wcscat(text, L"-text");
  if (std::wcscmp(text, L"wide-text") != 0 ||
      std::wcslen(text) != 9) return 12;

  wchar_t formatted[32];
  int parsed = 0;
  if (std::swprintf(formatted, 32, L"value=%d", 42) != 8 ||
      std::wcscmp(formatted, L"value=42") != 0 ||
      std::swscanf(formatted, L"value=%d", &parsed) != 1 || parsed != 42) {
    return 13;
  }

  std::tm time = {};
  time.tm_year = 126;
  time.tm_mon = 8;
  time.tm_mday = 2;
  if (std::wcsftime(formatted, 32, L"%Y-%m-%d", &time) != 10 ||
      std::wcscmp(formatted, L"2026-09-02") != 0) return 14;
  return 0;
}
