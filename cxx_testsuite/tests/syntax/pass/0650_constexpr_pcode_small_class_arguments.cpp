// RUN: -std=c++20 -fconstexpr-eval=pcode

// A class argument is passed at its exact size. Rounding a one-byte class up
// to a word made the next argument land in the padding and read as zero.

struct Day {
  unsigned char value;
  constexpr explicit Day(unsigned v) : value(static_cast<unsigned char>(v)) {}
  constexpr operator unsigned() const { return value; }
  friend constexpr bool operator==(Day left, Day right) {
    return static_cast<unsigned>(left) == static_cast<unsigned>(right);
  }
};

constexpr int both(Day left, Day right) {
  return static_cast<int>(static_cast<unsigned>(left) * 100u +
                          static_cast<unsigned>(right));
}

static_assert(both(Day(2), Day(9)) == 209);
static_assert(Day(29) == Day(29));

constexpr Day named(29);
static_assert(named == Day(29));
static_assert(both(named, Day(7)) == 2907);

int main() { return 0; }
