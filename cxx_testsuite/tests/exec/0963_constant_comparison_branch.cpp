// RUN: -std=c++17
// EXPECT_EXIT: 0
// A comparison of a constexpr local against a constant, used directly as a
// branch condition in a member function, compares two constants once the
// local is folded.  The branch must not test an unassigned register.
struct B {
  int Eq() const { constexpr int ti = 0; return ti == 0 ? 100 : ti; }
  int Ne() const { constexpr int ti = 0; return ti != 0 ? ti : 100; }
  int NotTaken() const { constexpr int ti = 3; return ti == 0 ? 100 : ti; }
  int If() const {
    constexpr int ti = 0;
    if (ti == 0) return 100;
    return ti;
  }
  template <typename U> int Sized() const {
    constexpr int ti = sizeof(U) == 1 ? 0 : 2;
    return ti == 0 ? 100 : ti;
  }
};

int main() {
  B b;
  if (b.Eq() != 100) return 1;
  if (b.Ne() != 100) return 2;
  if (b.NotTaken() != 3) return 3;
  if (b.If() != 100) return 4;
  if (b.Sized<char>() != 100 || b.Sized<int>() != 2) return 5;
  return 0;
}
