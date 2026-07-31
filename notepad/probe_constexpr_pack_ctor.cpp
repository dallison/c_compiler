template <int... Values>
struct holder {
  int stored[2]{};

  template <class First, class Second>
  constexpr holder(First first, Second second) {
    stored[0] = first;
    stored[1] = second;
  }
};

constexpr holder<1, 2> value(3, 4);
static_assert(value.stored[0] == 3);
static_assert(value.stored[1] == 4);
