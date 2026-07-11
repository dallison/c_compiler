// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <map>
#include <utility>

struct PairValue {
  int left;
  int right;

  PairValue() : left(0), right(0) {}
  PairValue(int l, int r) : left(l), right(r) {}
};

int range_and_initializer_construction() {
  std::pair<const int, int> input[3] = {
      std::pair<const int, int>(2, 20),
      std::pair<const int, int>(1, 10),
      std::pair<const int, int>(2, 99)};
  std::map<int, int> from_range(input, input + 3);
  if (from_range.size() != 2 || from_range.find(1)->second != 10 ||
      from_range.find(2)->second != 20) {
    return 1;
  }

  std::map<int, int> from_list = {
      std::pair<const int, int>(4, 40),
      std::pair<const int, int>(3, 30)};
  if (from_list.size() != 2 || from_list.begin()->first != 3) {
    return 2;
  }

  std::initializer_list<std::pair<const int, int> > one = {
      std::pair<const int, int>(7, 70)};
  from_list = one;
  if (from_list.size() != 1 || from_list.find(7)->second != 70) {
    return 3;
  }
  return 0;
}

int insertion_overloads() {
  std::map<int, int> values;
  values.insert(values.end(), std::pair<const int, int>(1, 10));
  values.emplace_hint(values.end(), 2, 20);
  values.insert({std::pair<const int, int>(3, 30),
                 std::pair<const int, int>(2, 99)});
  if (values.size() != 3 || values.find(2)->second != 20) {
    return 1;
  }

  std::pair<std::map<int, int>::iterator, bool> inserted =
      values.try_emplace(4, 40);
  std::pair<std::map<int, int>::iterator, bool> duplicate =
      values.try_emplace(4, 400);
  if (!inserted.second || duplicate.second || values.find(4)->second != 40) {
    return 2;
  }

  std::pair<std::map<int, int>::iterator, bool> assigned =
      values.insert_or_assign(4, 44);
  std::pair<std::map<int, int>::iterator, bool> added =
      values.insert_or_assign(5, 50);
  if (assigned.second || !added.second || values.find(4)->second != 44 ||
      values.find(5)->second != 50) {
    return 3;
  }

  std::map<int, PairValue> pair_values;
  pair_values.try_emplace(1, 6, 7);
  if (pair_values.find(1)->second.left != 6 ||
      pair_values.find(1)->second.right != 7) {
    return 4;
  }
  return 0;
}

int main() {
  if (range_and_initializer_construction() != 0) {
    return 1;
  }
  if (insertion_overloads() != 0) {
    return 2;
  }
  return 0;
}
