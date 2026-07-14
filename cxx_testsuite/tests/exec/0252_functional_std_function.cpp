// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <functional>
#include <type_traits>

static int add_one(int value) {
  return value + 1;
}

struct Object {
  int value;
  int add(int amount) const { return value + amount; }
};

struct Stateful {
  int value;
  int operator()(int amount) { return value += amount; }
};

struct Large {
  int values[16];
  int operator()(int index) const { return values[index]; }
};

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
  int operator()() const { return 1; }
};

struct CopyOnly {
  int value;
  CopyOnly(int initial) : value(initial) {}
  CopyOnly(const CopyOnly&) = default;
  CopyOnly(CopyOnly&&) = delete;
  int operator()(int amount) const { return value + amount; }
};

struct DeducedFunctor {
  long operator()(short value) const noexcept { return value + 5; }
};

struct VolatileFunctor {
  int operator()(int value) volatile { return value - 1; }
};

static_assert(std::is_default_constructible<std::function<int()>>::value);
static_assert(std::is_copy_constructible<std::function<int()>>::value);
static_assert(std::is_move_constructible<std::function<int()>>::value);
static_assert(std::is_nothrow_move_constructible<std::function<int()>>::value);
static_assert(!std::is_constructible<std::function<int()>, MoveOnly>::value);
static_assert(
    std::is_constructible<std::function<int(int)>, CopyOnly&>::value);

int main(void) {
  std::function<int(int)> function_pointer = &add_one;
  if (!function_pointer || function_pointer(3) != 4) return 1;
  if (function_pointer.target_type() != typeid(int (*)(int))) return 2;
  int (**pointer_target)(int) = function_pointer.target<int (*)(int)>();
  if (pointer_target == nullptr || *pointer_target != &add_one) return 3;
  if (function_pointer.target<Stateful>() != nullptr) return 4;

  int bias = 5;
  auto lambda = [bias](int value) { return bias + value; };
  std::function<int(int)> small = lambda;
  if (small(7) != 12) return 5;
  if (small.target<decltype(lambda)>() == nullptr) return 6;

  std::function<int(int)> copied = small;
  if (copied(8) != 13 || small(9) != 14) return 7;
  std::function<int(int)> moved = static_cast<std::function<int(int)>&&>(copied);
  if (moved(10) != 15) return 8;

  Stateful stateful{10};
  std::function<int(int)> reference = std::ref(stateful);
  if (reference(2) != 12 || stateful.value != 12) return 9;

  Large large{};
  for (int i = 0; i < 16; ++i) large.values[i] = i * 3;
  std::function<int(int)> heap = large;
  if (heap(11) != 33) return 10;
  const std::function<int(int)>& const_heap = heap;
  const Large* const_heap_target = const_heap.target<Large>();
  if (const_heap_target == nullptr || const_heap_target->values[7] != 21)
    return 21;
  std::function<int(int)> heap_copy = heap;
  Large* heap_copy_target = heap_copy.target<Large>();
  if (heap_copy_target == nullptr || heap_copy_target == const_heap_target ||
      heap_copy_target->values[7] != 21)
    return 22;
  if (heap_copy(7) != 21) return 11;
  std::function<int(int)> heap_moved =
      static_cast<std::function<int(int)>&&>(heap);
  if (heap_moved(11) != 33 || heap) return 23;

  Object object{20};
  std::function<int(const Object&, int)> member_function = &Object::add;
  if (member_function(object, 4) != 24) return 12;
  std::function<int&(Object&)> data_member = &Object::value;
  data_member(object) = 31;
  if (object.value != 31) return 13;

  std::function<void()> empty;
  if (empty || empty != nullptr || !(nullptr == empty)) return 14;
  bool caught = false;
  try {
    empty();
  } catch (const std::bad_function_call& error) {
    caught = error.what() != nullptr;
  }
  if (!caught) return 15;

  int (*null_pointer)(int) = nullptr;
  std::function<int(int)> null_function = null_pointer;
  if (null_function) return 16;

  moved.swap(function_pointer);
  if (moved(1) != 2 || function_pointer(2) != 7) return 17;
  function_pointer = nullptr;
  if (function_pointer) return 18;
  function_pointer = &add_one;
  if (function_pointer(8) != 9) return 19;

  std::function deduced = &add_one;
  if (deduced(12) != 13) return 20;

  std::function deduced_functor = DeducedFunctor{};
  static_assert(
      std::is_same<decltype(deduced_functor), std::function<long(short)>>::value);
  if (deduced_functor(7) != 12) return 25;

  std::function deduced_volatile = VolatileFunctor{};
  static_assert(std::is_same<decltype(deduced_volatile),
                             std::function<int(int)>>::value);
  if (deduced_volatile(9) != 8) return 27;

  auto ctad_lambda = [](int value) { return value * 2; };
  std::function deduced_lambda = ctad_lambda;
  static_assert(
      std::is_same<decltype(deduced_lambda), std::function<int(int)>>::value);
  if (deduced_lambda(6) != 12) return 26;

  CopyOnly copy_only{40};
  std::function<int(int)> copy_only_function = copy_only;
  if (copy_only_function(2) != 42) return 24;

  return 0;
}
