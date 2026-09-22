// RUN: -std=c++17
// EXPECT_EXIT: 0

template <typename T>
T identity(T v) {
  return v;
}

extern template int identity<int>(int v);

int main() { return identity<int>(0); }
