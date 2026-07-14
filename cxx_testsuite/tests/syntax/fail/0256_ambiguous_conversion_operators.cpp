// RUN: -std=c++20
// Two conversion operators reach the target through equally-ranked standard
// conversions (int -> long and char -> long are both integral conversions), so
// selecting a user-defined conversion is ambiguous and must be rejected.
// EXPECT: cannot convert from 'Ambiguous' to 'long'
struct Ambiguous {
  operator int() const { return 1; }
  operator char() const { return 2; }
};

int main() {
  Ambiguous a;
  long x = a;
  return static_cast<int>(x);
}
