// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct SizeProvider {
  using size_type = unsigned long;

  size_type max_size(void) const {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T>
unsigned long free_max_size(void) {
  using size_type = unsigned long;
  return static_cast<size_type>(-1) / sizeof(T);
}

template <class T>
bool unsigned_cast_compares_as_unsigned(void) {
  using size_type = unsigned long;
  return static_cast<size_type>(-1) > static_cast<size_type>(0);
}

int main(void) {
  SizeProvider<int> provider;
  unsigned long expected = 4611686018427387903UL;
  if (provider.max_size() != expected) {
    return 1;
  }
  if (free_max_size<int>() != expected) {
    return 2;
  }
  if (!unsigned_cast_compares_as_unsigned<int>()) {
    return 3;
  }
  return 0;
}
