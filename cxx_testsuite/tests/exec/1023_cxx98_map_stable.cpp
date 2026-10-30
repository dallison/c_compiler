// RUN: -std=c++98
// EXPECT_EXIT: 0

#include <map>

int main() {
  std::map<int, int> values;
  values[2] = 20;
  std::map<int, int>::iterator kept = values.find(2);
  int* stable = &kept->second;
  values[1] = 10;
  values[3] = 30;
  if (kept->first != 2 || *stable != 20) {
    return 1;
  }
  if (values.begin()->first != 1 || values.find(3)->second != 30 ||
      values.size() != 3) {
    return 2;
  }
  *stable = 21;
  if (values.find(2)->second != 21) {
    return 3;
  }
  if (values.erase(1) != 1 || kept->first != 2 || kept->second != 21 ||
      values.size() != 2 || values.begin()->first != 2) {
    return 4;
  }
  if (values.erase(2) != 1 || values.size() != 1 || values.begin()->first != 3) {
    return 5;
  }
  if (values.insert(std::pair<const int, int>(3, 99)).second) {
    return 6;
  }
  if (values.find(3)->second != 30) {
    return 7;
  }
  return 0;
}
