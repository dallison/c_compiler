// RUN: -std=c++23
// EXPECT_EXIT: 0

#define \N{GREEK SMALL LETTER PI} 17

int main() {
  const char assigned[] = "\N{LATIN CAPITAL LETTER A WITH MACRON}";
  if ((unsigned char)assigned[0] != 0xc4 ||
      (unsigned char)assigned[1] != 0x80 || assigned[2] != 0) {
    return 1;
  }

  const char aliases[] =
      "\N{HORIZONTAL TABULATION}"
      "\N{BYTE ORDER MARK}"
      "\N{PRESENTATION FORM FOR VERTICAL RIGHT WHITE LENTICULAR BRACKET}";
  if (aliases[0] != '\t' || (unsigned char)aliases[1] != 0xef ||
      (unsigned char)aliases[2] != 0xbb ||
      (unsigned char)aliases[3] != 0xbf ||
      (unsigned char)aliases[4] != 0xef ||
      (unsigned char)aliases[5] != 0xb8 ||
      (unsigned char)aliases[6] != 0x98 || aliases[7] != 0) {
    return 2;
  }

  const char16_t hangul[] = u"\N{HANGUL SYLLABLE GA}";
  const char32_t ideographs[] =
      U"\N{CJK UNIFIED IDEOGRAPH-4E00}"
      U"\N{TANGUT IDEOGRAPH-17000}"
      U"\N{NUSHU CHARACTER-1B170}"
      U"\N{KHITAN SMALL SCRIPT CHARACTER-18B00}";
  if (hangul[0] != 0xac00 || hangul[1] != 0 ||
      ideographs[0] != 0x4e00 || ideographs[1] != 0x17000 ||
      ideographs[2] != 0x1b170 || ideographs[3] != 0x18b00 ||
      ideographs[4] != 0) {
    return 3;
  }

  const char16_t face[] = u"\N{GRINNING FACE}";
  if (face[0] != 0xd83d || face[1] != 0xde00 || face[2] != 0) {
    return 4;
  }

  int \u03c6 = 9;
  if (\N{GREEK SMALL LETTER PHI} != 9 ||
      \N{GREEK SMALL LETTER PI} != 17) {
    return 5;
  }

  const char raw[] = R"(\N{LATIN CAPITAL LETTER A})";
  if (raw[0] != '\\' || raw[1] != 'N') {
    return 6;
  }
  return 0;
}
