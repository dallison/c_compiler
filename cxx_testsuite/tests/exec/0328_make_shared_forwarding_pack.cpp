// RUN: -std=c++23 -O1
// EXPECT_EXIT: 0

#include <memory>
#include <utility>

struct class_value {
  int value;

  explicit class_value(int input) : value(input) {}
  class_value(const class_value& other) : value(other.value) {}
};

struct observed_values {
  int first;
  int middle;
  int last;

  observed_values(const class_value& first_value, int middle_value,
                  class_value& last_value)
      : first(first_value.value),
        middle(middle_value),
        last(last_value.value) {}
};

template <class T, class... Args>
std::shared_ptr<T> build_shared(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

struct shared_holder {
  std::shared_ptr<observed_values> value;

  shared_holder(const class_value& first, int middle, class_value& last) {
    value = build_shared<observed_values>(first, middle, last);
  }
};

int main() {
  const class_value first(11);
  int middle = 17;
  class_value last(23);
  std::shared_ptr<observed_values> result =
      build_shared<observed_values>(first, middle, last);
  if (!result) return 1;
  if (result->first != 11) return 2;
  if (result->middle != 17) return 3;
  if (result->last != 23) return 4;

  std::shared_ptr<observed_values> assigned;
  assigned = build_shared<observed_values>(first, middle, last);
  if (!assigned) return 5;
  if (assigned->first != 11) return 6;
  if (assigned->middle != 17) return 7;
  if (assigned->last != 23) return 8;

  shared_holder holder(first, middle, last);
  if (!holder.value) return 9;
  if (holder.value->first != 11) return 10;
  if (holder.value->middle != 17) return 11;
  if (holder.value->last != 23) return 12;

  std::shared_ptr<observed_values> reset_value =
      build_shared<observed_values>(first, middle, last);
  reset_value.reset();
  if (reset_value) return 13;

  int pointed_value = 31;
  int* empty_pointer = nullptr;
  int* full_pointer = &pointed_value;
  std::swap(empty_pointer, full_pointer);
  if (empty_pointer != &pointed_value) return 14;
  if (full_pointer != nullptr) return 15;
  return 0;
}
