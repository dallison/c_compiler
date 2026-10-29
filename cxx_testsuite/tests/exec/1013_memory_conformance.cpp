// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <memory>
#include <stddef.h>
#include <type_traits>

static int deleted;

struct Box {
  int value;
  explicit Box(int v) : value(v) {}
  Box() : value(-1) {}
  ~Box() {}
};

struct Del {
  int id;
  explicit Del(int n) : id(n) {}
  Del(const Del& other) : id(other.id) {}
  Del(Del&& other) noexcept : id(other.id) { other.id = 0; }
  Del& operator=(Del&& other) noexcept {
    id = other.id;
    other.id = 0;
    return *this;
  }
  void operator()(Box* pointer) const {
    deleted += id;
    delete pointer;
  }
};

struct UsesArg {
  int value;
  using allocator_type = std::allocator<int>;
  UsesArg(std::allocator_arg_t, const std::allocator<int>&, int v) : value(v) {}
};

struct Over {
  alignas(64) char bytes[64];
};

int main() {
  {
    std::unique_ptr<Box, Del> source(new Box(3), Del(4));
    std::unique_ptr<Box, Del> moved(static_cast<std::unique_ptr<Box, Del>&&>(source));
    if (source || moved.get() == 0 || moved.get_deleter().id != 4) {
      return 1;
    }
    std::unique_ptr<Box, Del> assigned(static_cast<Box*>(0), Del(0));
    assigned = static_cast<std::unique_ptr<Box, Del>&&>(moved);
    if (moved || assigned.get_deleter().id != 4) {
      return 2;
    }
  }
  if (deleted != 4) {
    return 3;
  }

  std::unique_ptr<int[]> bits(new int[2]);
  if (!bits) {
    return 4;
  }
  bits[0] = 1;
  bits[1] = 2;
  if (bits[0] + bits[1] != 3) {
    return 5;
  }

  auto fresh = std::make_unique_for_overwrite<Box>();
  if (fresh->value != -1) {
    return 6;
  }
  auto raw = std::make_unique_for_overwrite<int[]>(3);
  if (!raw) {
    return 7;
  }

  alignas(Box) unsigned char raw_slot[sizeof(Box) * 2];
  Box* slot = reinterpret_cast<Box*>(raw_slot);
  Box values[2] = {Box(8), Box(9)};
  std::uninitialized_move(values, values + 2, slot);
  if (slot[0].value != 8 || slot[1].value != 9) {
    return 8;
  }
  std::destroy(slot, slot + 2);

  int storage[4];
  std::uninitialized_value_construct(storage, storage + 4);
  if (storage[0] != 0 || storage[3] != 0) {
    return 9;
  }

  void* buffer = storage;
  size_t space = sizeof(storage);
  void* aligned = std::align(4, 4, buffer, space);
  if (aligned == 0 || space > sizeof(storage)) {
    return 10;
  }
  int* assumed = std::assume_aligned<4>(storage);
  if (assumed != storage) {
    return 11;
  }

  auto many = std::make_shared<int[]>(3);
  if (many[0] != 0 || many[1] != 0 || many[2] != 0 || many.use_count() != 1) {
    return 12;
  }
  auto copied = std::make_shared<int[]>(2, 6);
  if (copied[0] != 6 || copied[1] != 6) {
    return 13;
  }
  auto fixed = std::make_shared<int[2]>();
  if (fixed[0] != 0 || fixed[1] != 0) {
    return 14;
  }
  auto blank = std::make_shared_for_overwrite<Box>();
  if (blank->value != -1) {
    return 15;
  }

  int live = 0;
  int* raw_int = new int(7);
  std::shared_ptr<int> counted(raw_int, [&live](int* p) {
    ++live;
    delete p;
  }, std::allocator<int>());
  counted.reset();
  if (live != 1) {
    return 16;
  }

  std::allocator<int> shared_alloc;
  auto allocated = std::allocate_shared<int[]>(shared_alloc, 3);
  if (allocated[0] != 0 || allocated[2] != 0 || allocated.use_count() != 1) {
    return 21;
  }
  auto filled = std::allocate_shared<int[]>(shared_alloc, 2, 5);
  if (filled[0] != 5 || filled[1] != 5) {
    return 22;
  }
  auto bounded = std::allocate_shared<int[2]>(shared_alloc, 4);
  if (bounded[0] != 4 || bounded[1] != 4) {
    return 23;
  }
  auto raw_boxes =
      std::allocate_shared_for_overwrite<Box[]>(shared_alloc, 1);
  if (raw_boxes[0].value != -1) {
    return 24;
  }

  UsesArg built = std::make_obj_using_allocator<UsesArg>(std::allocator<int>(), 11);
  if (built.value != 11) {
    return 17;
  }

  std::allocator<Over> alloc;
  Over* over = alloc.allocate(1);
  alloc.deallocate(over, 1);

  std::shared_ptr<int> left(new int(1));
  std::shared_ptr<int> right(new int(2));
  if (!(left < right) && !(right < left)) {
    return 18;
  }
  std::owner_less<void> order;
  if (order(left, left)) {
    return 19;
  }
  if (std::hash<std::shared_ptr<int>>()(left) !=
      std::hash<int*>()(left.get())) {
    return 20;
  }
  return 0;
}
