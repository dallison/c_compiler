// RUN: -std=c++20
#include <vector>

int main(void) {
  std::vector<int> values;
  values.reserve(4);
  values.push_back(1);
  values.push_back(2);
  values.resize(3, 9);
  values.insert(values.begin() + 1, 7);
  values.erase(values.begin() + 1);

  (void)values.rbegin();

  return 0;
}
