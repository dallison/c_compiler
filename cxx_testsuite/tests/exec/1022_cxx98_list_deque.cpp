// RUN: -std=c++98
// EXPECT_EXIT: 0

#include <deque>
#include <list>

struct Item {
  int value;
};

static int item_compares = 0;

bool operator<(const Item& left, const Item& right) {
  if (item_compares == 0) {
    throw 1;
  }
  --item_compares;
  return left.value < right.value;
}

static int expect_list(const std::list<int>& values, const int* expected,
                       int count) {
  if (static_cast<int>(values.size()) != count) {
    return 1;
  }
  std::list<int>::const_iterator it = values.begin();
  for (int i = 0; i < count; ++i) {
    if (it == values.end() || *it != expected[i]) {
      return 1;
    }
    ++it;
  }
  if (it != values.end()) {
    return 1;
  }
  return 0;
}

int main() {
  std::list<int> values;
  values.push_back(1);
  values.push_back(2);
  int* stable = &values.front();
  std::list<int>::iterator first = values.begin();
  values.push_front(0);
  if (*stable != 1 || *first != 1 || values.front() != 0 || values.back() != 2 ||
      values.size() != 3) {
    return 1;
  }

  std::list<int>::iterator kept = values.end();
  --kept;
  values.erase(first);
  if (*kept != 2 || *stable != 1 || values.size() != 2 || values.front() != 0) {
    return 2;
  }

  std::list<int> ordered;
  ordered.push_back(4);
  ordered.push_back(1);
  ordered.push_back(3);
  ordered.push_back(2);
  std::list<int>::iterator four = ordered.begin();
  ordered.sort();
  if (*four != 4) {
    return 3;
  }
  int sorted[] = {1, 2, 3, 4};
  if (expect_list(ordered, sorted, 4) != 0) {
    return 4;
  }

  std::list<int> extra;
  extra.push_back(5);
  extra.push_back(6);
  ordered.splice(ordered.end(), extra, extra.begin());
  if (extra.empty() || extra.size() != 1 || extra.front() != 6 ||
      ordered.back() != 5 || *four != 4) {
    return 5;
  }
  ordered.splice(ordered.end(), extra);
  if (!extra.empty() || ordered.back() != 6 || ordered.size() != 6) {
    return 6;
  }

  std::list<int> merged_left;
  std::list<int> merged_right;
  merged_left.push_back(1);
  merged_left.push_back(4);
  merged_right.push_back(2);
  merged_right.push_back(3);
  merged_left.merge(merged_right);
  int merged[] = {1, 2, 3, 4};
  if (!merged_right.empty() || expect_list(merged_left, merged, 4) != 0) {
    return 7;
  }

  std::list<int>::iterator one = merged_left.begin();
  merged_left.reverse();
  if (*one != 1 || merged_left.front() != 4 || merged_left.back() != 1) {
    return 8;
  }

  std::list<int> copied = merged_left;
  copied.pop_front();
  if (merged_left.front() != 4 || copied.front() != 3 || copied.size() != 3) {
    return 9;
  }

  std::list<int> dupes;
  dupes.push_back(1);
  dupes.push_back(1);
  dupes.push_back(2);
  dupes.push_back(2);
  dupes.push_back(2);
  dupes.unique();
  int unique_values[] = {1, 2};
  if (expect_list(dupes, unique_values, 2) != 0) {
    return 10;
  }
  dupes.remove(1);
  if (dupes.size() != 1 || dupes.front() != 2) {
    return 11;
  }

  std::list<Item> items;
  Item low;
  low.value = 1;
  Item high;
  high.value = 2;
  items.push_back(high);
  items.push_back(low);
  item_compares = 0;
  try {
    items.sort();
    return 12;
  } catch (int) {
  }
  if (items.size() != 2) {
    return 13;
  }
  int saw_low = 0;
  int saw_high = 0;
  for (std::list<Item>::iterator it = items.begin(); it != items.end(); ++it) {
    if (it->value == 1) {
      ++saw_low;
    }
    if (it->value == 2) {
      ++saw_high;
    }
  }
  if (saw_low != 1 || saw_high != 1) {
    return 14;
  }

  std::deque<int> deck;
  deck.push_back(1);
  deck.push_back(2);
  int* deck_stable = &deck.front();
  deck.push_front(0);
  if (*deck_stable != 1 || deck.front() != 0 || deck.back() != 2 || deck[1] != 1 ||
      deck.size() != 3) {
    return 15;
  }
  for (int i = 0; i < 40; ++i) {
    deck.push_front(i);
    deck.push_back(i);
  }
  if (*deck_stable != 1 || deck.front() != 39 || deck.back() != 39) {
    return 16;
  }

  std::deque<int> deck_copy = deck;
  deck_copy.pop_front();
  if (deck.front() != 39 || deck_copy.front() == 39) {
    return 17;
  }

  std::deque<int> middle;
  middle.push_back(1);
  middle.push_back(3);
  std::deque<int>::iterator slot = middle.begin();
  ++slot;
  middle.insert(slot, 2);
  if (middle.size() != 3 || middle[0] != 1 || middle[1] != 2 || middle[2] != 3) {
    return 18;
  }
  int* still = &middle[0];
  middle.insert(middle.begin(), 0);
  if (*still != 1 || middle.front() != 0 || middle[2] != 2) {
    return 19;
  }
  std::deque<int>::iterator drop = middle.begin();
  ++drop;
  middle.erase(drop);
  if (middle.size() != 3 || middle[0] != 0 || middle[1] != 2 || middle[2] != 3) {
    return 20;
  }
  return 0;
}
