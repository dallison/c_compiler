// RUN: -std=c++26
// EXPECT: A using-declarator cannot be reflected

namespace ns {
int value = 0;
}

using ns::value;

void test() {
  constexpr auto r = ^^value;
}
