// RUN: -std=c++20

namespace std {
template <class T>
struct initializer_list {
  const T* __begin;
  unsigned long __size;

  const T* begin() const {
    return __begin;
  }

  const T* end() const {
    return __begin + __size;
  }

  unsigned long size() const {
    return __size;
  }
};
}

int choose(std::initializer_list<int> values) {
  int sum = 0;
  const int* current = values.begin();
  while (current != values.end()) {
    sum = sum + *current;
    current = current + 1;
  }
  return 100 + sum;
}

int choose(int first, int second) {
  return 10 + first + second;
}

struct Box {
  int tag;

  Box(std::initializer_list<int> values) {
    tag = 200 + (int)values.size();
  }

  Box(int first, int second) {
    tag = first + second;
  }
};

struct AggregatePair {
  int first;
  int second;
};

int main(void) {
  std::initializer_list<int> values = {3, 4, 5};
  if (values.size() != 3) {
    return 1;
  }
  if (*values.begin() != 3 || *(values.begin() + 2) != 5) {
    return 2;
  }

  if (choose({1, 2}) != 103) {
    return 3;
  }

  Box box{1, 2};
  if (box.tag != 202) {
    return 4;
  }

  auto deduced = {6, 7, 8};
  if (deduced.size() != 3 || *(deduced.begin() + 1) != 7) {
    return 5;
  }

  AggregatePair aggregate = {9, 10};
  if (aggregate.first != 9 || aggregate.second != 10) {
    return 6;
  }

  return 0;
}
