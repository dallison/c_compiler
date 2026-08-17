// RUN: -std=c++26

#include <memory>
#include <new>
#include <version>

#if __cpp_constexpr != 202406L
#error "__cpp_constexpr must advertise C++26 placement new"
#endif

#if __cpp_lib_constexpr_new != 202406L
#error "__cpp_lib_constexpr_new must advertise C++26 placement new"
#endif

struct box {
  int value;
  constexpr box(int v) : value(v) {}
  constexpr ~box() {}
};

union choice {
  int integer;
  box object;

  constexpr choice() : integer(0) {}
  constexpr ~choice() {}
};

constexpr bool typed_void_round_trip() {
  int value = 42;
  void* erased = &value;
  int* restored = static_cast<int*>(erased);
  return restored == &value && *restored == 42;
}

constexpr bool replace_scalar_and_class() {
  int scalar = 1;
  int* scalar_result = new (static_cast<void*>(&scalar)) int(42);
  box object(1);
  box* object_result = new (static_cast<void*>(&object)) box(42);
  return scalar_result == &scalar && scalar == 42 &&
         object_result == &object && object.value == 42;
}

constexpr bool replace_subobject() {
  struct pair {
    box left;
    box right;
  } values = {box(1), box(2)};
  box* right = new (static_cast<void*>(&values.right)) box(41);
  return right == &values.right && values.right.value == 41;
}

constexpr bool replace_union_member() {
  choice selected;
  box* active = new (static_cast<void*>(&selected.object)) box(42);
  return active == &selected.object && selected.object.value == 42;
}

constexpr bool place_array() {
  int values[3] = {1, 2, 3};
  int* placed = new (static_cast<void*>(&values[0])) int[3];
  placed[0] = 10;
  placed[1] = 11;
  placed[2] = 21;
  return placed == &values[0] && values[0] + values[1] + values[2] == 42;
}

constexpr bool construct_and_destroy() {
  box storage(1);
  std::destroy_at(&storage);
  box* result = std::construct_at(&storage, 42);
  return result == &storage && storage.value == 42;
}

constexpr bool allocator_storage() {
  std::allocator<int> alloc;
  int* storage = alloc.allocate(1);
  int* result = new (static_cast<void*>(storage)) int(42);
  bool ok = result == storage && *result == 42;
  std::destroy_at(result);
  alloc.deallocate(storage, 1);
  return ok;
}

constexpr bool allocator_allocate_deallocate() {
  std::allocator<int> alloc;
  int* storage = alloc.allocate(1);
  bool ok = storage != nullptr;
  alloc.deallocate(storage, 1);
  return ok;
}

static_assert(typed_void_round_trip());
static_assert(replace_scalar_and_class());
static_assert(replace_subobject());
static_assert(replace_union_member());
static_assert(place_array());
static_assert(construct_and_destroy());
static_assert(allocator_allocate_deallocate());
static_assert(allocator_storage());
