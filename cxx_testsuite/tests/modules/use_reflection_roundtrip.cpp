import reflection_roundtrip;

using ImportedBox = typename [:reflected_box:];
using ImportedToken = ReflectionToken<^^ReflectedBox<int>>;

static_assert(reflected_box == ^^ReflectedBox<int>);

template <class>
struct IsExportedToken {
  static constexpr bool value = false;
};

template <>
struct IsExportedToken<ExportedReflectionToken> {
  static constexpr bool value = true;
};

static_assert(IsExportedToken<ImportedToken>::value);

int main() {
  ImportedBox box{42};
  return box.value == 42 ? 0 : 1;
}
