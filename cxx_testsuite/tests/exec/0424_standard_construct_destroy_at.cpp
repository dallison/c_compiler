// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <memory>
#include <version>

#if __cpp_lib_constexpr_dynamic_alloc != 201907L
#error "__cpp_lib_constexpr_dynamic_alloc must advertise construct_at"
#endif

struct tracked {
  static int destruction_sum;
  int value;

  tracked(int left, int right) : value(left + right) {}
  ~tracked() { destruction_sum += value; }
};

int tracked::destruction_sum = 0;

struct constexpr_tracked {
  int value;

  constexpr explicit constexpr_tracked(int initial) : value(initial) {}
  constexpr ~constexpr_tracked() {}
};

constexpr bool reconstruct_in_constant_expression() {
  constexpr_tracked value(1);
  std::destroy_at(&value);
  constexpr_tracked* result = std::construct_at(&value, 42);
  return result == &value && value.value == 42;
}

static_assert(reconstruct_in_constant_expression());

union object_storage {
  tracked object;

  object_storage() {}
  ~object_storage() {}
};

union array_storage {
  tracked objects[2];

  array_storage() {}
  ~array_storage() {}
};

int main() {
  object_storage one;
  tracked* object = std::construct_at(&one.object, 19, 23);
  if (object != &one.object || object->value != 42) {
    return 1;
  }
  std::destroy_at(object);
  if (tracked::destruction_sum != 42) {
    return 2;
  }

  array_storage two;
  std::construct_at(&two.objects[0], 10, 11);
  std::construct_at(&two.objects[1], 9, 12);
  std::destroy_at(&two.objects);
  return tracked::destruction_sum == 84 ? 0 : 3;
}
