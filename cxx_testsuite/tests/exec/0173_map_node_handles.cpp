// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <map>
#include <utility>

int extract_and_insert() {
  std::map<int, int> values;
  values.insert(std::pair<const int, int>(1, 10));
  values.insert(std::pair<const int, int>(2, 20));

  std::map<int, int>::node_type node = values.extract(1);
  if (node.empty() || values.contains(1) || values.size() != 1) {
    return 1;
  }
  node.key() = 3;
  node.mapped() = 30;

  std::map<int, int> target;
  std::map<int, int>::insert_return_type inserted =
      target.insert(std::move(node));
  if (!inserted.inserted || !inserted.node.empty() ||
      inserted.position->first != 3 || target.find(3)->second != 30) {
    return 2;
  }

  std::map<int, int>::node_type duplicate = values.extract(2);
  duplicate.key() = 3;
  std::map<int, int>::insert_return_type rejected =
      target.insert(std::move(duplicate));
  if (rejected.inserted || rejected.node.empty() ||
      rejected.position->second != 30 || rejected.node.mapped() != 20) {
    return 3;
  }

  rejected.node.key() = 4;
  std::map<int, int>::iterator hint_result =
      target.insert(target.end(), std::move(rejected.node));
  if (hint_result->first != 4 || target.find(4)->second != 20) {
    return 4;
  }
  return 0;
}

int extract_iterator_and_merge() {
  std::map<int, int> left;
  left.insert(std::pair<const int, int>(1, 10));
  left.insert(std::pair<const int, int>(3, 30));

  std::map<int, int>::node_type first = left.extract(left.begin());
  if (first.key() != 1 || left.contains(1) || left.size() != 1) {
    return 1;
  }

  std::map<int, int, std::less<void> > right;
  right.insert(std::pair<const int, int>(2, 20));
  right.insert(std::pair<const int, int>(3, 300));
  right.insert(std::pair<const int, int>(4, 40));

  left.merge(right);
  if (left.size() != 3 || right.size() != 1 || !right.contains(3) ||
      left.find(2)->second != 20 || left.find(4)->second != 40) {
    return 2;
  }
  if (left.contains(3) && left.find(3)->second != 30) {
    return 3;
  }
  return 0;
}

int main() {
  if (extract_and_insert() != 0) {
    return 1;
  }
  if (extract_iterator_and_merge() != 0) {
    return 2;
  }
  return 0;
}
