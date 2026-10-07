// RUN: -std=c++20
// EXPECT_EXIT: 0

// Block-scope multidimensional arrays of class type are destroyed element by
// element, last first, whether the block is left by falling off its end or by
// a return.  A string literal brace-elided into an array whose elements are
// not characters initializes one element.

#include <string>

int order[64];
int count;

struct T {
  int id;
  T(int i = 0) : id(i) {}
  ~T() { order[count++] = id; }
};

int Jump(int k) {
  T t[2][3] = {{1, 2, 3}, {4, 5, 6}};
  if (k) {
    return t[1][2].id;
  }
  return 0;
}

int main() {
  {
    std::string s[2][2];
    s[1][1] = "x";
  }
  {
    std::string s[][2] = {"a", "b", "c"};
    if (sizeof(s) != 4 * sizeof(std::string) || s[1][0] != "c" ||
        !s[1][1].empty()) {
      return 1;
    }
  }
  {
    std::string s[2][2] = {"a", "b", "c"};
    if (s[0][1] != "b" || s[1][0] != "c") {
      return 2;
    }
  }
  {
    const char* p[][2] = {"a", "b", "c"};
    if (sizeof(p) != 4 * sizeof(const char*) || p[1][0][0] != 'c' ||
        p[1][1] != nullptr) {
      return 3;
    }
  }
  {
    T t[2][2] = {{1, 2}, {3, 4}};
  }
  if (Jump(1) != 6) {
    return 4;
  }
  static const int kExpected[] = {4, 3, 2, 1, 6, 5, 4, 3, 2, 1};
  if (count != 10) {
    return 5;
  }
  for (int i = 0; i < count; i++) {
    if (order[i] != kExpected[i]) {
      return 10 + i;
    }
  }
  return 0;
}
