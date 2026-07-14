// RUN: -std=c++20
#include <functional>
#include <typeinfo>

static int add_one(int value) {
  return value + 1;
}

static int side_effect;

static int record_value(int value) {
  side_effect = value;
  return value + 10;
}

struct Counter {
  int value;

  int add(int amount) {
    value += amount;
    return value;
  }
};

struct LargeCallable {
  int values[8];

  int operator()(int amount) {
    return values[0] + amount;
  }
};

int main() {
  std::function<int(int)> empty;
  if (empty || empty.target_type() != typeid(void)) return 1;
  if (!(empty == nullptr) || !(nullptr == empty)) return 15;

  bool threw = false;
  try {
    empty(1);
  } catch (const std::bad_function_call& error) {
    threw = error.what()[0] == 's';
  }
  if (!threw) return 2;

  std::function<int(int)> function_pointer = add_one;
  if (!function_pointer || function_pointer(4) != 5) return 3;
  std::function deduced_function = add_one;
  using expected_deduced_type = std::function<int(int)>;
  static_assert(std::is_same<decltype(deduced_function),
                             expected_deduced_type>::value);
  if (deduced_function(2) != 3) return 22;
  if (function_pointer.target<int (*)(int)>() == nullptr) return 4;
  if (function_pointer.target_type() == typeid(LargeCallable)) return 21;
  if (function_pointer.target<LargeCallable>() != nullptr) return 16;
  const std::function<int(int)>& const_function = function_pointer;
  if (const_function(5) != 6 ||
      const_function.target<int (*)(int)>() == nullptr) return 17;

  std::function<void(int)> void_function = record_value;
  void_function(6);
  if (side_effect != 6) return 18;

  int captured = 3;
  std::function<int(int)> lambda =
      [captured](int value) mutable { return ++captured + value; };
  if (lambda(2) != 6 || lambda(2) != 7) return 5;

  std::function<int(int)> copied = lambda;
  if (copied(1) != 7 || lambda(1) != 7) return 6;
  copied = copied;
  if (copied(1) != 8) return 19;

  std::function<int(int)> moved = static_cast<std::function<int(int)>&&>(copied);
  if (copied || moved(2) != 10) return 7;

  LargeCallable large{{10, 0, 0, 0, 0, 0, 0, 0}};
  std::function<int(int)> heap_target = large;
  if (heap_target(5) != 15) return 8;
  if (heap_target.target<LargeCallable>() == nullptr) return 9;

  Counter counter{1};
  std::function<int(Counter&, int)> member_function = &Counter::add;
  if (member_function(counter, 4) != 5) return 10;

  std::function<int&(Counter&)> member_object = &Counter::value;
  member_object(counter) = 20;
  if (counter.value != 20) return 11;

  int referenced = 2;
  auto reference_callable = [&referenced](int amount) {
    referenced += amount;
    return referenced;
  };
  std::function<int(int)> wrapped = std::ref(reference_callable);
  if (wrapped(3) != 5 || referenced != 5) return 12;

  function_pointer = nullptr;
  if (function_pointer || !(function_pointer == nullptr)) return 13;
  function_pointer.swap(empty);
  if (function_pointer || empty) return 20;
  function_pointer = add_one;
  function_pointer.swap(moved);
  if (function_pointer(1) != 10 || moved(2) != 3) return 14;

  return 0;
}
