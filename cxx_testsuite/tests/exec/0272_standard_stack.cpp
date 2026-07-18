// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <stack>
#include <deque>
#include <vector>
#include <string>
#include <utility>

static int basics() {
  std::stack<int> s;
  if (!s.empty() || s.size() != 0) return 1;
  for (int i = 0; i < 100; i++) s.push(i);
  if (s.size() != 100 || s.top() != 99) return 2;
  int sum = 0;
  while (!s.empty()) {
    sum += s.top();
    s.pop();
  }
  if (sum != 4950) return 3;  // sum 0..99
  return 0;
}

static int lifo_order() {
  std::stack<int> s;
  int in[] = {5, 3, 9, 1, 7};
  for (int i = 0; i < 5; i++) s.push(in[i]);
  int expect = 4;
  while (!s.empty()) {
    if (s.top() != in[expect--]) return 10;
    s.pop();
  }
  if (expect != -1) return 11;
  return 0;
}

static int emplace_and_strings() {
  std::stack<std::string> s;
  s.emplace("abc");
  s.emplace(3, 'x');  // "xxx"
  if (s.size() != 2 || s.top() != "xxx") return 20;
  s.pop();
  if (s.top() != "abc") return 21;
  return 0;
}

static int over_vector() {
  std::stack<int, std::vector<int>> s;
  s.push(1);
  s.push(2);
  s.push(3);
  if (s.size() != 3 || s.top() != 3) return 30;
  s.pop();
  if (s.top() != 2) return 31;
  return 0;
}

static int copy_and_compare() {
  std::stack<int> a;
  for (int i = 0; i < 5; i++) a.push(i);
  std::stack<int> b = a;
  if (b != a) return 40;
  b.pop();
  if (b == a) return 41;
  if (!(b < a)) return 42;  // b is a prefix, fewer elements

  std::stack<int> c;
  c = a;
  if (c != a) return 43;

  using std::swap;
  swap(b, c);
  if (b.size() != a.size()) return 44;
  return 0;
}

int main() {
  int rc;
  if ((rc = basics()) != 0) return rc;
  if ((rc = lifo_order()) != 0) return rc;
  if ((rc = emplace_and_strings()) != 0) return rc;
  if ((rc = over_vector()) != 0) return rc;
  if ((rc = copy_and_compare()) != 0) return rc;
  return 0;
}
