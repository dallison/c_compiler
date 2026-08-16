export module reflection_extended;

export inline constexpr auto reflected_int = ^^int;

export namespace detail {
inline constexpr int value = 7;
}

export namespace alias_ns = detail;

export template <typename T>
struct primary {
  T item;
};

export template [: ^^primary :] struct secondary {
  T item;
};
