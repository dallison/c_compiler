// RUN: -std=c++20

// In-class `static constexpr` data members whose own type is the enclosing
// class are accepted both with a functional-cast initializer and with a braced
// initializer, and an out-of-class definition with a matching const-qualified
// type (and no initializer of its own) is also accepted.
struct Cat {
  int _v;
  constexpr explicit Cat(signed char v) : _v(v) {}
  constexpr bool negative() const { return _v < 0; }

  static constexpr Cat less = Cat((signed char)-1);
  static constexpr Cat greater{1};
};

constexpr Cat Cat::greater;

static_assert(Cat::less.negative(), "less must be negative");
static_assert(!Cat::greater.negative(), "greater must be non-negative");
