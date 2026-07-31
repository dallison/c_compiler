export module template_surface;

export namespace template_probe {

template <class T>
struct Box {
  T value;

  Box(T initial) : value(initial) {}

  template <class U>
  T add(U increment) const;

  int pick(int input) const { return input + 1; }
  double pick(double input) const { return input + 0.5; }
};

template <class T>
template <class U>
T Box<T>::add(U increment) const {
  static_assert(sizeof(U) > 0, "member template argument must be complete");
  return value + static_cast<T>(increment);
}

template <class T>
Box(T) -> Box<T>;

template <class T>
struct Category {
  static constexpr int value = 0;
};

template <class T>
struct Category<T*> {
  static constexpr int value = 1;
};

template <>
struct Category<int> {
  static constexpr int value = 2;
};

template <class T>
inline constexpr int variable_category = 0;

template <class T>
inline constexpr int variable_category<T*> = 1;

template <int Value>
struct Number {
  static constexpr int value = Value;
  using type = Number<Value>;

  constexpr int get() const { return Value; }
};

template <int Left, int Right>
using NumberSum = typename Number<Left + Right>::type;

template <class T>
constexpr bool has_addition() {
  return requires(T value) { value + value; };
}

}  // namespace template_probe
