// A user-defined conversion operator that returns a *class* type by value must
// actually be invoked by casts and implicit conversions, even when the source
// and target classes happen to be layout-compatible.  (Previously the front end
// treated any two struct/union types as "equal ignoring sign" and silently
// reinterpreted the source object, bypassing the operator and producing the
// wrong value for a value-changing conversion.)

struct Fahrenheit {
  int degrees;
};

struct Celsius {
  int degrees;
  // Value-changing conversion: C -> F is degrees * 9 / 5 + 32.
  operator Fahrenheit() const { return Fahrenheit{degrees * 9 / 5 + 32}; }
};

static Fahrenheit take(Fahrenheit f) { return f; }
static Fahrenheit ret(Celsius c) { return c; }  // implicit conversion on return

int main() {
  Celsius boiling{100};  // 100C == 212F

  // C-style cast.
  if (((Fahrenheit)boiling).degrees != 212) return 1;

  // static_cast.
  if (static_cast<Fahrenheit>(boiling).degrees != 212) return 2;

  // Copy-initialization.
  Fahrenheit a = boiling;
  if (a.degrees != 212) return 3;

  // Implicit conversion when passed as an argument.
  if (take(boiling).degrees != 212) return 4;

  // Implicit conversion on return.
  if (ret(boiling).degrees != 212) return 5;

  // A genuine same-type copy must still work (non-regression).
  Fahrenheit b = a;
  if (b.degrees != 212) return 6;

  // Another temperature for good measure (0C == 32F).
  Celsius freezing{0};
  if (((Fahrenheit)freezing).degrees != 32) return 7;

  return 0;
}
