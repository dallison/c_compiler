// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Holder {
  static const char* text() {
    return R"cc(
      hello
    )cc";
  }
};

int main() {
  const char* text = Holder::text();
  for (const char* p = text; *p != '\0'; p++) {
    if (p[0] == 'h' && p[1] == 'e' && p[2] == 'l' && p[3] == 'l' &&
        p[4] == 'o') {
      return 0;
    }
  }
  return 1;
}
