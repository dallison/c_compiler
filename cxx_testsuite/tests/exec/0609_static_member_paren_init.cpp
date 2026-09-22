// RUN: -std=c++17
// EXPECT_EXIT: 0

struct CordRepExternal {
  int size;
  explicit constexpr CordRepExternal(int n) : size(n) {}
};

template <typename Str>
struct ConstInitExternalStorage {
  static CordRepExternal value;
};

struct Hello {
  static constexpr int value = 7;
};

template <typename Str>
CordRepExternal ConstInitExternalStorage<Str>::value(Str::value);

int main() {
  return ConstInitExternalStorage<Hello>::value.size == 7 ? 0 : 1;
}
