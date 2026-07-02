// RUN: -std=c++20
#include <vector>

int main(void) {
  std::vector<int> values;
  if (!values.empty() || values.size() != 0 || values.capacity() != 0) {
    return 1;
  }
  values.reserve(4);
  if (values.capacity() < 4) {
    return 2;
  }
  values.push_back(1);
  values.push_back(2);
  values.push_back(3);
  if (values.size() != 3 || values[0] != 1 || values.back() != 3) {
    return 3;
  }
  if (*values.rbegin() != 3) {
    return 4;
  }
  values.resize(5, 9);
  if (values.size() != 5 || values[3] != 9 || values[4] != 9) {
    return 5;
  }
  values.insert(values.begin() + 1, 7);
  if (values.size() != 6 || values[1] != 7 || values[2] != 2) {
    return 6;
  }
  values.erase(values.begin() + 1);
  if (values.size() != 5 || values[1] != 2) {
    return 7;
  }

  return 0;
}
