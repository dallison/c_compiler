// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <queue>
#include <deque>
#include <vector>
#include <string>
#include <functional>
#include <utility>

static int queue_basics() {
  std::queue<int> q;
  if (!q.empty() || q.size() != 0) return 1;
  for (int i = 0; i < 100; i++) q.push(i);
  if (q.front() != 0 || q.back() != 99 || q.size() != 100) return 2;
  int prev = -1;
  while (!q.empty()) {
    if (q.front() != prev + 1) return 3;
    prev = q.front();
    q.pop();
  }
  if (prev != 99) return 4;
  return 0;
}

static int queue_emplace_and_copy() {
  std::queue<std::string> q;
  q.push("a");
  q.emplace("b");
  if (q.front() != "a" || q.back() != "b") return 10;

  std::queue<int> a;
  for (int i = 0; i < 5; i++) a.push(i);
  std::queue<int> b = a;
  if (b != a) return 11;
  b.pop();
  if (b == a) return 12;

  using std::swap;
  swap(a, b);
  if (a.size() != 4 || b.size() != 5) return 13;
  return 0;
}

static int priority_queue_max() {
  std::priority_queue<int> pq;
  int data[] = {3, 1, 4, 1, 5, 9, 2, 6};
  for (int i = 0; i < 8; i++) pq.push(data[i]);
  if (pq.top() != 9) return 20;
  int last = 1000;
  int count = 0;
  while (!pq.empty()) {
    if (pq.top() > last) return 21;  // non-increasing order
    last = pq.top();
    pq.pop();
    count++;
  }
  if (count != 8) return 22;
  return 0;
}

static int priority_queue_min() {
  int data[] = {3, 1, 4, 1, 5, 9, 2, 6};
  std::priority_queue<int, std::vector<int>, std::greater<int>> minpq;
  for (int i = 0; i < 8; i++) minpq.push(data[i]);
  if (minpq.top() != 1) return 30;
  int prev = -1000;
  int count = 0;
  while (!minpq.empty()) {
    if (minpq.top() < prev) return 31;  // non-decreasing order
    prev = minpq.top();
    minpq.pop();
    count++;
  }
  if (count != 8) return 32;
  return 0;
}

static int priority_queue_from_range() {
  int data[] = {3, 1, 4, 1, 5, 9, 2, 6};
  std::vector<int> v;
  for (int i = 0; i < 8; i++) v.push_back(data[i]);
  std::priority_queue<int> pq(v.begin(), v.end());
  if (pq.top() != 9 || pq.size() != 8) return 40;
  pq.push(100);
  if (pq.top() != 100) return 41;
  pq.emplace(50);
  if (pq.top() != 100) return 42;
  return 0;
}

int main() {
  int rc;
  if ((rc = queue_basics()) != 0) return rc;
  if ((rc = queue_emplace_and_copy()) != 0) return rc;
  if ((rc = priority_queue_max()) != 0) return rc;
  if ((rc = priority_queue_min()) != 0) return rc;
  if ((rc = priority_queue_from_range()) != 0) return rc;
  return 0;
}
