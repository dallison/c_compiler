// RUN: -std=c++17
// EXPECT_EXIT: 0

namespace absl {
template <typename T>
class Vec {
 public:
  int n;
  explicit Vec(int v) : n(v) {}

 private:
  template <typename U>
  friend int inspect(const absl::Vec<U>& v);
};

template <typename U>
int inspect(const absl::Vec<U>& v) {
  return v.n;
}
}  // namespace absl

int main() {
  absl::Vec<int> v(4);
  return inspect(v) - 4;
}
