// RUN: -std=c++26
// EXPECT_EXIT: 0

// C++26 placement new and conversions from void* on allocated class objects,
// evaluated by p-code.
#include <memory>
#include <new>

struct P {
  int a;
  int b;
  constexpr P(int x, int y) : a(x), b(y) {}
};

constexpr int placement_into_allocator_storage() {
  std::allocator<P> alloc;
  P* storage = alloc.allocate(2);
  P* first = ::new (static_cast<void*>(storage)) P(1, 2);
  P* second = ::new (static_cast<void*>(storage + 1)) P(30, 40);
  int result = first->a + first->b + second->a + second->b;
  std::destroy_at(second);
  std::destroy_at(first);
  alloc.deallocate(storage, 2);
  return result;
}

constexpr int construct_at_in_allocator_storage() {
  std::allocator<P> alloc;
  P* storage = alloc.allocate(3);
  for (int i = 0; i < 3; ++i) {
    std::construct_at(storage + i, i, 10 * i);
  }
  int result = 0;
  for (int i = 0; i < 3; ++i) {
    result += storage[i].a + storage[i].b;
  }
  for (int i = 0; i < 3; ++i) {
    std::destroy_at(storage + i);
  }
  alloc.deallocate(storage, 3);
  return result;
}

constexpr int new_class_objects() {
  P* p = new P(5, 6);
  P* array = new P[2]{P(1, 1), P(2, 2)};
  int result = p->a * p->b + array[0].a + array[1].b;
  delete p;
  delete[] array;
  return result;
}

constexpr int void_round_trip_to_heap_object() {
  P* p = new P(7, 8);
  void* erased = p;
  P* restored = static_cast<P*>(erased);
  int result = restored->a + restored->b;
  delete p;
  return result;
}

static_assert(placement_into_allocator_storage() == 73);
static_assert(construct_at_in_allocator_storage() == 33);
static_assert(new_class_objects() == 33);
static_assert(void_round_trip_to_heap_object() == 15);

int main() {
  if (placement_into_allocator_storage() != 73) return 1;
  if (construct_at_in_allocator_storage() != 33) return 2;
  if (new_class_objects() != 33) return 3;
  if (void_round_trip_to_heap_object() != 15) return 4;
  return 0;
}
