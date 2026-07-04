// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <initializer_list>
#include <vector>

int main(void) {
  std::vector<int> values;
  values.assign(3, 7);
  if (values.size() != 3 || values[0] != 7 || values[2] != 7) {
    return 1;
  }

  std::initializer_list<int> first_list = {1, 2, 3};
  values = first_list;
  if (values.size() != 3 || values[0] != 1 || values[2] != 3) {
    return 2;
  }

  std::initializer_list<int> second_list = {4, 5};
  values.assign(second_list);
  if (values.size() != 2 || values[0] != 4 || values[1] != 5) {
    return 3;
  }

  int more[3] = {8, 9, 10};
  std::vector<int> from_range(more, more + 3);
  if (from_range.size() != 3 || from_range[0] != 8 || from_range[2] != 10) {
    return 4;
  }

  values.insert(values.begin() + 1, more, more);
  if (values.size() != 2 || values[0] != 4 || values[1] != 5) {
    return 5;
  }

  values.insert(values.end(), from_range.begin(), from_range.end());
  if (values.size() != 5 || values[2] != 8 || values[4] != 10) {
    return 6;
  }

  values.insert(values.begin() + 2, values.begin(), values.begin() + 2);
  if (values.size() != 7) {
    return 7;
  }
  if (values[0] != 4 || values[1] != 5 || values[2] != 4 ||
      values[3] != 5 || values[4] != 8 || values[6] != 10) {
    return 8;
  }

  std::initializer_list<int> insert_list = {12, 13};
  values.insert(values.begin() + 3, insert_list);
  if (values.size() != 9 || values[3] != 12 || values[4] != 13 ||
      values[5] != 5) {
    return 9;
  }

  return 0;
}
