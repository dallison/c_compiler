// RUN: -std=c++23
// EXPECT_EXIT: 0

int main() {
  const char ordinary[] = "café";
  if (sizeof(ordinary) != 6 || ordinary[0] != 'c' ||
      ordinary[1] != 'a' || ordinary[2] != 'f' ||
      (unsigned char)ordinary[3] != 0xc3 ||
      (unsigned char)ordinary[4] != 0xa9 || ordinary[5] != 0) {
    return 1;
  }

  const char16_t utf16[] = u"©🙂";
  if (sizeof(utf16) != 8 || utf16[0] != 0x00a9 ||
      utf16[1] != 0xd83d || utf16[2] != 0xde42 || utf16[3] != 0) {
    return 2;
  }

  const char32_t utf32[] = U"©🙂";
  if (sizeof(utf32) != 12 || utf32[0] != 0x00a9 ||
      utf32[1] != 0x1f642 || utf32[2] != 0) {
    return 3;
  }
  return 0;
}
