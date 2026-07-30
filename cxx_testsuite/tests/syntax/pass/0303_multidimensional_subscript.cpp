// RUN: -std=c++23

#if __cpp_multidimensional_subscript != 202211L
#error "__cpp_multidimensional_subscript has the wrong value"
#endif

struct grid {
  constexpr int operator[](int row, int column) const {
    return row * 10 + column;
  }
};

struct variadic_grid {
  template <class... Indices>
  constexpr int operator[](Indices... indices) const {
    return sizeof...(indices);
  }
};

struct zero_index {
  constexpr int operator[]() const {
    return 7;
  }
};

struct static_grid {
  static constexpr int operator[](int first, int second) {
    return first + second;
  }
};

template <class View, class... Indices>
constexpr int read(View& view, Indices... indices) {
  return view[indices...];
}

static_assert(grid{}[2, 3] == 23);
static_assert(variadic_grid{}[1, 2, 3] == 3);
static_assert(zero_index{}[] == 7);
static_assert(static_grid{}[4, 5] == 9);

constexpr bool check_dependent_subscript() {
  grid value;
  return read(value, 4, 5) == 45;
}

static_assert(check_dependent_subscript());

int values[] = {2, 3, 5};

int parenthesized_comma_subscript() {
  return values[(0, 2)];
}
