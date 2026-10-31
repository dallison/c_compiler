// RUN: -std=c++98
// EXPECT_EXIT: 0

#include <algorithm>
#include <functional>
#include <queue>
#include <vector>

bool max_heap(const int* first, const int* last) {
  int length = static_cast<int>(last - first);
  for (int index = 0; index < length; ++index) {
    int left = 2 * index + 1;
    int right = left + 1;
    if (left < length && first[index] < first[left]) return false;
    if (right < length && first[index] < first[right]) return false;
  }
  return true;
}

struct OpenQueue : std::priority_queue<int> {
  int front_value() const { return c.front(); }
};

int main() {
  int values[] = {3, 1, 4, 1, 5, 9, 2, 6};
  std::make_heap(values, values + 8);
  if (values[0] != 9 || !max_heap(values, values + 8)) return 1;
  std::pop_heap(values, values + 8);
  if (values[7] != 9 || !max_heap(values, values + 7)) return 2;
  values[7] = 10;
  std::push_heap(values, values + 8);
  if (values[0] != 10 || !max_heap(values, values + 8)) return 3;
  std::sort_heap(values, values + 8);
  for (int index = 0; index < 7; ++index) {
    if (values[index] > values[index + 1]) return 4;
  }

  OpenQueue open;
  open.push(2);
  open.push(9);
  open.push(4);
  if (open.top() != 9 || open.front_value() != 9) return 5;
  open.pop();
  if (open.top() != 4 || open.front_value() != 4) return 6;
  open.pop();
  if (open.top() != 2 || open.front_value() != 2) return 7;

  std::priority_queue<int, std::vector<int>, std::greater<int> > minimum;
  minimum.push(2);
  minimum.push(9);
  minimum.push(4);
  if (minimum.top() != 2) return 8;
  minimum.pop();
  if (minimum.top() != 4) return 9;

  int source[] = {1, 5, 3, 4};
  std::vector<int> initial(source, source + 4);
  std::priority_queue<int> from_container(std::less<int>(), initial);
  if (from_container.top() != 5) return 10;
  from_container.pop();
  if (from_container.top() != 4) return 11;

  std::priority_queue<int> from_range(source, source + 4);
  if (from_range.top() != 5) return 12;
  from_range.pop();
  if (from_range.top() != 4) return 13;
  return 0;
}
