// RUN: -std=c++23 -Werror=preprocessor
// EXPECT: literal %s MESSAGE

#define MESSAGE expanded
#warning literal %s MESSAGE

int main() {
  return 0;
}
