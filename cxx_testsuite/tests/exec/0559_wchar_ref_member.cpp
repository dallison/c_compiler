// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Iter {
  mutable wchar_t value_;

  const wchar_t& operator*() const { return value_; }
};

int main() {
  Iter it;
  it.value_ = L'A';
  const wchar_t& r = *it;
  return r == L'A' ? 0 : 1;
}
