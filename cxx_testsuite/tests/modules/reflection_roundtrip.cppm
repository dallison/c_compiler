export module reflection_roundtrip;

export template <class T>
struct ReflectedBox {
  T value;
};

export constexpr auto reflected_box = ^^ReflectedBox<int>;

export template <auto reflection>
struct ReflectionToken {
  static constexpr auto value = reflection;
};

export using ExportedReflectionToken = ReflectionToken<reflected_box>;
