export module reflection;

export inline constexpr auto reflected_int = ^^int;

export template <auto reflection>
consteval bool reflects_int() {
  return reflection == ^^int;
}
