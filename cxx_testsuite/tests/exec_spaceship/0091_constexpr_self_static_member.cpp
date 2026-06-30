// Regression test for two related fixes around in-class `static constexpr`
// data members whose own type is the enclosing class:
//
//   * A functional-cast initializer (`T((signed char)-1)`) -- not just a braced
//     initializer -- is constant-evaluated, even though the member's value
//     constructor is only fully formed once the class is complete.
//
//   * An out-of-class definition (`constexpr T T::m;`) re-declares the member
//     with a matching const-qualified type and needs no initializer of its own.

struct Cat {
  int _v;
  constexpr explicit Cat(signed char v) : _v(v) {}
  constexpr bool negative() const { return _v < 0; }
  constexpr int value() const { return _v; }

  // Functional-cast initializer (exercises the deferred constructor
  // evaluation).
  static constexpr Cat less = Cat((signed char)-1);
  // Braced initializer with an out-of-class definition below.
  static constexpr Cat greater{1};
};

// Deprecated-but-legal out-of-class definition: no initializer here.
constexpr Cat Cat::greater;

// Used in a constant expression to prove the value was folded, not constructed
// dynamically at run time.
static_assert(Cat::less.negative(), "less must be negative");
static_assert(Cat::less.value() == -1, "less must be -1");
static_assert(Cat::greater.value() == 1, "greater must be 1");
static_assert(!Cat::greater.negative(), "greater must be non-negative");

int main() {
  if (!Cat::less.negative()) return 1;
  if (Cat::less.value() != -1) return 2;
  if (Cat::greater.value() != 1) return 3;
  if (Cat::greater.negative()) return 4;
  return 0;
}
