// RUN: -std=c++98
// EXPECT_EXIT: 0

#include <algorithm>
#include <bitset>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <vector>

int main() {
  int source[] = {4, 1, 3, 2};
  std::vector<int> values(source, source + 4);
  std::sort(values.begin(), values.end());
  if (values.front() != 1 || values.back() != 4) return 1;

  std::deque<int> deque;
  deque.push_back(2);
  deque.push_front(1);
  if (deque.front() != 1 || deque.back() != 2) return 2;

  std::list<int> list;
  list.push_back(3);
  list.push_front(1);
  if (list.front() != 1 || list.back() != 3) return 3;

  std::map<int, int> map;
  map[2] = 20;
  map[1] = 10;
  if (map.begin()->first != 1 || map.find(2)->second != 20) return 4;

  std::set<int> set;
  set.insert(3);
  set.insert(1);
  if (*set.begin() != 1 || set.count(3) != 1) return 5;

  std::queue<int> queue;
  queue.push(7);
  if (queue.front() != 7) return 6;

  std::stack<int> stack;
  stack.push(8);
  if (stack.top() != 8) return 7;

  std::priority_queue<int> priority;
  priority.push(2);
  priority.push(9);
  if (priority.top() != 9) return 8;

  std::bitset<8> bits(5);
  if (!bits.test(0) || bits.test(1) || !bits.test(2)) return 9;

  std::less<int> less;
  if (!less(1, 2)) return 10;
  return 0;
}
