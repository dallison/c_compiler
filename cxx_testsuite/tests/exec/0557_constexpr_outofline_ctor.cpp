// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Wide {
  unsigned long lo_;
  unsigned long hi_;
  constexpr Wide(unsigned long high, unsigned long low);
  constexpr Wide(int v);
};

constexpr Wide::Wide(unsigned long high, unsigned long low)
    : lo_{low}, hi_{high} {}

constexpr Wide::Wide(int v)
    : lo_{static_cast<unsigned long>(v)}, hi_{0} {}

int main() {
  Wide a(1, 2);
  Wide b(3);
  if (a.lo_ != 2 || a.hi_ != 1) {
    return 1;
  }
  if (b.lo_ != 3 || b.hi_ != 0) {
    return 2;
  }
  constexpr Wide c(4, 5);
  if (c.lo_ != 5 || c.hi_ != 4) {
    return 3;
  }
  return 0;
}
