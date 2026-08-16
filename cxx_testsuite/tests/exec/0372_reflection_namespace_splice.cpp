// RUN: -std=c++26
// EXPECT_EXIT: 0

namespace ns {
int value = 5;
}

constexpr auto ns_info = ^^ns;

int main() {
  return [:ns_info:]::value == 5 ? 0 : 1;
}
