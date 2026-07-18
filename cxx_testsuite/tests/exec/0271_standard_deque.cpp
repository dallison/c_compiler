// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <deque>
#include <string>
#include <initializer_list>
#include <utility>

static int basics() {
  std::deque<int> d;
  if (!d.empty() || d.size() != 0) return 1;
  for (int i = 0; i < 1000; i++) d.push_back(i);
  for (int i = 0; i < 1000; i++) d.push_front(-i - 1);
  if (d.size() != 2000) return 2;
  // front half is -1000..-1 (reversed pushes), then 0..999
  if (d.front() != -1000) return 3;
  if (d.back() != 999) return 4;
  if (d[1000] != 0) return 5;
  if (d.at(0) != -1000) return 6;
  int sum = 0;
  for (std::deque<int>::iterator it = d.begin(); it != d.end(); ++it) sum += *it;
  // sum of -1000..999 = -500500 + 499500 = -1000
  if (sum != -1000) return 7;
  // random access iterator arithmetic
  if (d.end() - d.begin() != 2000) return 8;
  if (*(d.begin() + 1000) != 0) return 9;
  return 0;
}

static int reverse_iteration() {
  std::deque<int> d;
  for (int i = 0; i < 5; i++) d.push_back(i);  // 0 1 2 3 4
  int expect = 4;
  for (std::deque<int>::reverse_iterator it = d.rbegin(); it != d.rend();
       ++it) {
    if (*it != expect--) return 10;
  }
  if (expect != -1) return 11;
  // const reverse iteration
  const std::deque<int>& cd = d;
  expect = 4;
  for (std::deque<int>::const_reverse_iterator it = cd.rbegin();
       it != cd.rend(); ++it) {
    if (*it != expect--) return 12;
  }
  // reverse iterator random access
  if (d.rend() - d.rbegin() != 5) return 13;
  if (*(d.rbegin() + 2) != 2) return 14;
  return 0;
}

static int pop_churn() {
  // Use as a queue: push_back / pop_front many times; must stay correct.
  std::deque<int> d;
  int expected = 0;
  int next = 0;
  for (int round = 0; round < 5000; round++) {
    d.push_back(next++);
    d.push_back(next++);
    if (d.front() != expected++) return 20;
    d.pop_front();
  }
  while (!d.empty()) {
    if (d.front() != expected++) return 21;
    d.pop_front();
  }
  if (expected != next) return 22;
  return 0;
}

static int insert_erase() {
  std::deque<int> d;
  for (int i = 0; i < 10; i++) d.push_back(i);  // 0..9
  d.insert(d.begin() + 5, 99);                  // 0 1 2 3 4 99 5 6 7 8 9
  if (d.size() != 11 || d[5] != 99 || d[6] != 5) return 30;
  d.erase(d.begin() + 5);                        // back to 0..9
  if (d.size() != 10 || d[5] != 5) return 31;
  d.erase(d.begin() + 2, d.begin() + 5);         // remove 2,3,4
  if (d.size() != 7 || d[2] != 5) return 32;
  // emplace in the middle
  std::deque<int>::iterator it = d.emplace(d.begin() + 1, 42);
  if (*it != 42 || d[1] != 42) return 33;
  return 0;
}

static int assign_resize_clear() {
  std::deque<int> d;
  d.assign(4, 7);
  if (d.size() != 4) return 40;
  for (int v : d) {
    if (v != 7) return 41;
  }
  std::deque<int> src = {1, 2, 3};
  d.assign(src.begin(), src.end());
  if (d.size() != 3 || d[0] != 1 || d[2] != 3) return 42;
  d.resize(5);
  if (d.size() != 5 || d[3] != 0 || d[4] != 0) return 43;
  d.resize(2);
  if (d.size() != 2 || d.back() != 2) return 44;
  d.resize(4, 9);
  if (d.size() != 4 || d.back() != 9) return 45;
  d.clear();
  if (!d.empty()) return 46;
  return 0;
}

static int strings_and_copy() {
  std::deque<std::string> d;
  d.push_back("hello");
  d.push_front("world");
  d.emplace_back("!!!");
  if (d.size() != 3) return 50;
  if (d[0] != "world" || d[1] != "hello" || d[2] != "!!!") return 51;
  std::deque<std::string> copy = d;
  if (copy != d) return 52;
  copy.pop_back();
  if (copy == d) return 53;
  std::deque<std::string> moved = std::move(d);
  if (moved.size() != 3 || moved[0] != "world") return 54;
  return 0;
}

static int init_and_compare() {
  std::deque<int> a = {1, 2, 3};
  std::deque<int> b = {1, 2, 4};
  if (!(a < b)) return 60;
  if (b < a) return 61;
  if (!(a != b)) return 62;
  std::deque<int> c = {1, 2, 3};
  if (!(a == c)) return 63;

  using std::swap;
  swap(a, b);
  if (a.back() != 4 || b.back() != 3) return 64;
  return 0;
}

int main() {
  int rc;
  if ((rc = basics()) != 0) return rc;
  if ((rc = reverse_iteration()) != 0) return rc;
  if ((rc = pop_churn()) != 0) return rc;
  if ((rc = insert_erase()) != 0) return rc;
  if ((rc = assign_resize_clear()) != 0) return rc;
  if ((rc = strings_and_copy()) != 0) return rc;
  if ((rc = init_and_compare()) != 0) return rc;
  return 0;
}
