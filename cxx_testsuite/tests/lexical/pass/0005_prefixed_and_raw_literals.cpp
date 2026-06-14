// RUN: -std=c++11
int main(void) {
  u8"text";
  u"text";
  U"text";
  L"text";
  u8'x';
  u'x';
  U'x';
  L'x';
  R"raw(first line
second line)raw";
  u8R"(raw)";
  LR"(raw)";
  return 0;
}
