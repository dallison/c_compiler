// RUN: -std=c++26

#include <initializer_list>

template <class T>
struct member_template_owner {
  template <class Iterator>
  void consume(Iterator first, Iterator last) {
    for (; first != last; ++first) {
      T value = *first;
      (void)value;
    }
  }

  void consume_list(std::initializer_list<T> values) {
    consume(values.begin(), values.end());
  }
};

void member_template_call_is_instantiated() {
  member_template_owner<int> owner;
  owner.consume_list({1, 2, 3});
}
