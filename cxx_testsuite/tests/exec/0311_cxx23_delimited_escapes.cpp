// RUN: -std=c++23
// EXPECT_EXIT: 0

int character_kind(char16_t) {
  return 16;
}

int character_kind(unsigned short) {
  return 2;
}

int character_kind(char32_t) {
  return 32;
}

int character_kind(unsigned int) {
  return 4;
}

int main() {
  static_assert(sizeof(char16_t) == 2);
  static_assert(sizeof(char32_t) == 4);
  if (character_kind(u'a') != 16 || character_kind((unsigned short)'a') != 2 ||
      character_kind(U'a') != 32 || character_kind((unsigned int)'a') != 4) {
    return 8;
  }

  const char text[] = "\x{41}\o{102}\u{43}";
  if (sizeof(text) != 4 || text[0] != 'A' || text[1] != 'B' ||
      text[2] != 'C' || text[3] != 0) {
    return 1;
  }
  if ('\x{44}' != 'D' || '\o{105}' != 'E' || '\u{46}' != 'F') {
    return 2;
  }

  const char8_t utf8[] = u8"\u{00a9}";
  if (sizeof(utf8) != 3 || utf8[0] != 0xc2 || utf8[1] != 0xa9 ||
      utf8[2] != 0) {
    return 3;
  }

  const char16_t utf16[] = u"\u{00a9}\u{1f642}";
  if (sizeof(utf16) != 8 || utf16[0] != 0x00a9 ||
      utf16[1] != 0xd83d || utf16[2] != 0xde42 || utf16[3] != 0) {
    return 4;
  }

  const char32_t utf32[] = U"\u{00a9}\u{1f642}";
  if (sizeof(utf32) != 12 || utf32[0] != 0x00a9 ||
      utf32[1] != 0x1f642 || utf32[2] != 0) {
    return 5;
  }

  const char16_t raw16[] = uR"(©)";
  const char32_t raw32[] = UR"(🙂)";
  if (sizeof(raw16) != 4 || raw16[0] != 0x00a9 || raw16[1] != 0 ||
      sizeof(raw32) != 8 || raw32[0] != 0x1f642 || raw32[1] != 0) {
    return 6;
  }

  if (u'\u{00a9}' != 0x00a9 || U'\u{1f642}' != 0x1f642 ||
      L'\u{00a9}' != 0x00a9) {
    return 7;
  }
  return 0;
}
