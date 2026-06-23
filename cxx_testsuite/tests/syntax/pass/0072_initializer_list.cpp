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

int choose_list(std::initializer_list<int> values);
int choose_list(int first, int second);

struct ListBox {
  int tag;
  ListBox(std::initializer_list<int> values);
  ListBox(int first, int second);
};

struct AggregatePair {
  int first;
  int second;
};

void use_initializer_list_semantics() {
  std::initializer_list<int> values = {1, 2, 3};
  int function_choice = choose_list({1, 2});
  ListBox box{1, 2};
  auto deduced = {4, 5, 6};
  AggregatePair aggregate = {7, 8};
  int scalar = {9};

  (void)values;
  (void)function_choice;
  (void)box;
  (void)deduced;
  (void)aggregate;
  (void)scalar;
}
