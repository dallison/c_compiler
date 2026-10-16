// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <coroutine>
#include <type_traits>

template <class T, class = void>
struct has_promise_type : std::false_type {};

template <class T>
struct has_promise_type<T, std::void_t<typename T::promise_type>>
    : std::true_type {};

struct Empty {};

struct Marker {};

struct WithPromise {
  using promise_type = Marker;
};

int used_traits;

struct WrongPromise;
struct TraitsPromise;

struct Task {
  using promise_type = WrongPromise;
  int value;
};

namespace std {
template <>
struct coroutine_traits<Task> {
  using promise_type = TraitsPromise;
};
}

struct WrongPromise {
  Task get_return_object() { return Task{2}; }
  std::suspend_never initial_suspend() { return {}; }
  std::suspend_never final_suspend() { return {}; }
  void return_void() { used_traits = 2; }
  void unhandled_exception() {}
};

struct TraitsPromise {
  Task get_return_object() {
    return Task{1};
  }
  std::suspend_never initial_suspend() { return {}; }
  std::suspend_never final_suspend() { return {}; }
  void return_void() { used_traits = 1; }
  void unhandled_exception() {}
};

Task overridden() {
  co_return;
}

int main() {
  if (has_promise_type<std::coroutine_traits<int>>::value) {
    return 1;
  }
  if (has_promise_type<std::coroutine_traits<Empty>>::value) {
    return 2;
  }
  if (has_promise_type<std::coroutine_traits<int, char>>::value) {
    return 3;
  }
  if (!has_promise_type<std::coroutine_traits<WithPromise>>::value) {
    return 4;
  }
  if (!std::is_same<std::coroutine_traits<WithPromise>::promise_type,
                    Marker>::value) {
    return 5;
  }
  if (!std::is_same<std::coroutine_traits<Task>::promise_type,
                    TraitsPromise>::value) {
    return 6;
  }

  Task result = overridden();
  if (result.value != 1 || used_traits != 1) {
    return 7;
  }
  return 0;
}
