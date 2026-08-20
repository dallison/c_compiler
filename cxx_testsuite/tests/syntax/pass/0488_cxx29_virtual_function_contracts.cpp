// RUN: -std=c++29

struct interface {
  virtual int compute(const int value)
      pre (value >= 0)
      post (result: result >= value) = 0;
};

struct implementation : interface {
  int compute(const int value) override
      pre (value < 100)
      post (result: result == value + 1) {
    return value + 1;
  }
};

struct constexpr_interface {
  virtual constexpr int evaluate(const int value) const
      pre (value == 4)
      post (result: result == 8) {
    return value;
  }
};

struct constexpr_implementation : constexpr_interface {
  constexpr int evaluate(const int value) const override
      pre (value > 0)
      post (result: result == value * 2) {
    return value * 2;
  }
};

constexpr int evaluate_through_interface() {
  constexpr_implementation object;
  const constexpr_interface& base = object;
  return base.evaluate(4);
}

static_assert(evaluate_through_interface() == 8);

int main() {
  implementation object;
  interface& base = object;
  return base.compute(4) == 5 ? 0 : 1;
}
