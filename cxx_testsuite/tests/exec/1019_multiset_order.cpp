// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <set>

struct Item {
  int key;
  int tag;
};

bool operator<(const Item& left, const Item& right) {
  return left.key < right.key;
}

bool operator==(const Item& left, const Item& right) {
  return left.key == right.key && left.tag == right.tag;
}

static int expect_item(const std::multiset<Item>& values, int index, int key,
                       int tag) {
  std::multiset<Item>::const_iterator it = values.begin();
  for (int i = 0; i < index; ++i) {
    if (it == values.end()) {
      return 1;
    }
    ++it;
  }
  if (it == values.end() || it->key != key || it->tag != tag) {
    return 1;
  }
  return 0;
}

int main() {
  std::multiset<Item> values;
  values.insert(Item{2, 1});
  values.insert(Item{1, 0});
  values.insert(Item{2, 2});
  values.insert(Item{3, 0});
  values.emplace(2, 3);
  if (values.size() != 5) {
    return 1;
  }
  if (expect_item(values, 0, 1, 0) || expect_item(values, 1, 2, 1) ||
      expect_item(values, 2, 2, 2) || expect_item(values, 3, 2, 3) ||
      expect_item(values, 4, 3, 0)) {
    return 2;
  }
  if (values.count(Item{2, -1}) != 3 || !values.contains(Item{1, 9})) {
    return 3;
  }
  std::pair<std::multiset<Item>::iterator, std::multiset<Item>::iterator>
      range = values.equal_range(Item{2, 0});
  int span = 0;
  for (std::multiset<Item>::iterator it = range.first; it != range.second;
       ++it) {
    ++span;
  }
  if (span != 3 || range.first->tag != 1) {
    return 4;
  }
  std::multiset<Item>::iterator middle = range.first;
  ++middle;
  if (middle->tag != 2) {
    return 5;
  }
  values.erase(middle);
  if (values.size() != 4 || expect_item(values, 1, 2, 1) ||
      expect_item(values, 2, 2, 3)) {
    return 6;
  }
  if (values.erase(Item{2, 0}) != 2 || values.size() != 2) {
    return 7;
  }
  if (expect_item(values, 0, 1, 0) || expect_item(values, 1, 3, 0)) {
    return 8;
  }
  if (values.find(Item{2, 0}) != values.end()) {
    return 9;
  }

  std::multiset<int> numbers = {3, 1, 2, 1};
  if (numbers.size() != 4 || *numbers.begin() != 1 || *--numbers.end() != 3) {
    return 10;
  }
  std::multiset<int> copied = numbers;
  copied.erase(1);
  if (numbers.size() != 4 || copied.size() != 2 || *copied.begin() != 2) {
    return 11;
  }

  std::multiset<int> tree;
  for (int i = 0; i < 32; ++i) {
    tree.insert(i);
    tree.insert(i);
  }
  if (tree.size() != 64 || tree.erase(0) != 2) {
    return 12;
  }
  for (int i = 1; i < 32; i += 2) {
    std::multiset<int>::iterator found = tree.find(i);
    if (found == tree.end() || tree.erase(found) == tree.end()) {
      return 13;
    }
  }
  if (tree.size() != 46) {
    return 14;
  }
  int expect = 1;
  int copies = 0;
  for (std::multiset<int>::iterator it = tree.begin(); it != tree.end();
       ++it) {
    if (*it != expect) {
      return 15;
    }
    ++copies;
    int want = (expect % 2 == 0) ? 2 : 1;
    if (copies == want) {
      ++expect;
      copies = 0;
    }
  }
  if (expect != 32 || copies != 0) {
    return 16;
  }
  tree.insert(0);
  tree.insert(0);
  tree.insert(32);
  if (*tree.begin() != 0 || *--tree.end() != 32 || tree.count(0) != 2 ||
      tree.lower_bound(2) == tree.end() || *tree.lower_bound(2) != 2 ||
      *tree.upper_bound(2) != 3) {
    return 17;
  }
  tree.clear();
  if (!tree.empty() || tree.begin() != tree.end()) {
    return 18;
  }
  tree.insert(4);
  tree.insert(4);
  tree.insert(1);
  std::multiset<int> other;
  other.insert(9);
  tree.swap(other);
  if (tree.size() != 1 || *tree.begin() != 9 || other.size() != 3 ||
      *other.begin() != 1 || other.count(4) != 2) {
    return 19;
  }
  return 0;
}
