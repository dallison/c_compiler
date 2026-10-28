// RUN: -std=c++23
// EXPECT_EXIT: 0
// C++23 string_view range constructor and the deleted nullptr constructor.
#include <string>
#include <string_view>

int main() {
  std::string owned("range");
  std::string_view view(owned);
  if (view.size() != owned.size() || view.data() != owned.data()) {
    return 1;
  }
  if (view != "range") {
    return 2;
  }
  const char raw[] = {'a', 'b', 'c'};
  std::string_view from_array(raw);
  if (from_array.size() != 3 || from_array[2] != 'c') {
    return 3;
  }
  return 0;
}
