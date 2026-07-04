// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <compare>
#include <vector>

struct Key {
  int value;

  explicit Key(int v) : value(v) {}
};

bool operator==(const Key& a, const Key& b) {
  return a.value == b.value;
}

std::strong_ordering operator<=>(const Key& a, const Key& b) {
  if (a.value < b.value) {
    return std::strong_ordering::less;
  }
  if (a.value > b.value) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

int test_custom_element_spaceship(void) {
  std::vector<Key> a;
  a.push_back(Key(1));
  a.push_back(Key(2));

  std::vector<Key> b;
  b.push_back(Key(1));
  b.push_back(Key(2));

  std::vector<Key> c;
  c.push_back(Key(1));
  c.push_back(Key(3));

  std::vector<Key> prefix;
  prefix.push_back(Key(1));

  std::vector<Key> longer;
  longer.push_back(Key(1));
  longer.push_back(Key(2));
  longer.push_back(Key(0));

  if (!(a == b) || a != b || !(a != c)) {
    return 1;
  }
  if (!((a <=> b) == 0) || !((a <=> c) < 0) || !((c <=> a) > 0)) {
    return 2;
  }
  if (!((prefix <=> a) < 0) || !((longer <=> a) > 0)) {
    return 3;
  }
  if (!(prefix < a) || !(c > a) || !(a <= b) || !(a >= b)) {
    return 4;
  }
  return 0;
}

int test_empty_custom_element_spaceship(void) {
  std::vector<Key> empty_a;
  std::vector<Key> empty_b;
  std::vector<Key> non_empty;
  non_empty.push_back(Key(1));

  if (!(empty_a == empty_b) || empty_a != empty_b) {
    return 10;
  }
  if (!((empty_a <=> empty_b) == 0) || !((empty_a <=> non_empty) < 0) ||
      !((non_empty <=> empty_a) > 0)) {
    return 11;
  }
  if (!(empty_a < non_empty) || !(non_empty > empty_a) ||
      !(empty_a <= empty_b) || !(empty_a >= empty_b)) {
    return 12;
  }
  return 0;
}

int main(void) {
  int result = test_custom_element_spaceship();
  if (result != 0) {
    return result;
  }
  result = test_empty_custom_element_spaceship();
  if (result != 0) {
    return result;
  }
  return 0;
}
