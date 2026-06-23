// RUN: -std=c++20
// EXPECT: Ambiguous class template argument deduction
template <typename T>
struct AmbiguousGuide {
  T value;
};

AmbiguousGuide(int) -> AmbiguousGuide<int>;
AmbiguousGuide(int) -> AmbiguousGuide<long>;

int main(void) {
  AmbiguousGuide value(1);
  return 0;
}
