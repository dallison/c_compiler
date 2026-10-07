// RUN: -std=c++17 -fconstexpr-eval=audit
// A conditional operator arm whose lowering reuses an existing register (an
// extension of an already-extended load) must still write the merge value.

constexpr int TwoArms(int i) {
  char ch = static_cast<char>(i);
  int u = (ch >= 'a' ? ch - 32 : ch);
  int l = (ch <= 'Z' ? ch + 32 : ch);
  return l * 1000 + u;
}
static_assert(TwoArms('a') == 97065);
static_assert(TwoArms('A') == 97065);
static_assert(TwoArms('5') == 85053);

constexpr int SignedArms(int i) {
  signed char c = static_cast<signed char>(i);
  int a = (i > 1000 ? 7 : c);
  int b = (i < -1000 ? 9 : c);
  return a * 1000 + b;
}
static_assert(SignedArms(-56) == -56056);

constexpr int ShortArms(int i) {
  unsigned short s = static_cast<unsigned short>(i);
  int a = (i > 100000 ? 0 : s);
  int b = (i > 200000 ? 1 : s);
  return a + b;
}
static_assert(ShortArms(70000) == 8928);

constexpr int IncrementArms(int i) {
  int x = i;
  int a = (i > 100 ? 0 : ++x);
  int b = (i > 100 ? 1 : ++x);
  return a * 100 + b;
}
static_assert(IncrementArms(3) == 405);

constexpr double FloatingArms(int i) {
  float f = static_cast<float>(i);
  double d = i;
  float a = (i > 100 ? 0.0f : f);
  double b = (i > 100 ? 1.0 : d + 1);
  return a * 100 + b;
}
static_assert(FloatingArms(2) == 203.0);

constexpr bool AsciiCaseFold() {
  for (int i = 0; i < 256; ++i) {
    char ch = static_cast<char>(i);
    char upper = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - 32) : ch;
    char lower = (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + 32) : ch;
    if ((i >= 'a' && i <= 'z' && (upper != i - 32 || lower != ch)) ||
        (i >= 'A' && i <= 'Z' && (lower != i + 32 || upper != ch))) {
      return false;
    }
  }
  return true;
}
static_assert(AsciiCaseFold());

int main() { return 0; }
