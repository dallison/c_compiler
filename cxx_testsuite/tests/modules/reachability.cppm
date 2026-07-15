export module reachability;

namespace detail {
template <typename T>
T add_offset(T value) {
  return value + 3;
}

struct Hidden {
  int value;
};

int unrelated_implementation_detail() {
  return 99;
}
}  // namespace detail

export template <typename T>
T add_hidden(T value) {
  return detail::add_offset(value);
}

export template <typename T>
T forwarded_template(T value);

template <typename T>
T forwarded_template(T value) {
  return detail::add_offset(value);
}

export detail::Hidden make_hidden() {
  detail::Hidden result;
  result.value = 7;
  return result;
}

export int ordinary_uses_hidden() {
  return detail::unrelated_implementation_detail();
}

export int visible_overload(int value) {
  return value + 1;
}

int visible_overload(short value) {
  return (int)value + 100;
}

module :private;

int private_implementation_detail() {
  return 41;
}
