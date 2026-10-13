// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <utility>

struct Value {
  int number;
};

int observed;

void Record(int&& number, Value*&& value) {
  observed = number + value->number;
}

template <typename T>
struct Traits {
  template <typename... Args>
  static void Construct(Args&&... args) {
    Record(std::forward<Args>(args)...);
  }
};

struct Tag {};

template <typename T>
struct Base {
  template <typename... Args>
  Base(Tag, Args&&... args) {
    Traits<T>::Construct(std::forward<Args>(args)...);
  }
};

template <typename T>
struct Derived : Base<T> {
  using Base<T>::Base;
};

struct Factory {
  template <typename T, typename... Args>
  static T Make(Args&&... args) {
    return T(Tag{}, std::forward<Args>(args)...);
  }
};

int main() {
  Value value{6};
  auto derived = Factory::Make<Derived<Value*>>(1, &value);
  (void)derived;
  return observed == 7 ? 0 : 1;
}
