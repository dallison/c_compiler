// RUN: -std=c++20
// EXPECT_EXIT: 0

// std::destroy, std::destroy_n and their std::ranges forms end the lifetime
// of each element exactly once, both in constant evaluation and at run time.
#include <memory>
#include <ranges>

struct Counted {
  int* destroyed;
  constexpr explicit Counted(int* counter) : destroyed(counter) {}
  constexpr ~Counted() { ++*destroyed; }
};

constexpr int destroy_with(int which) {
  int destroyed = 0;
  std::allocator<Counted> alloc;
  Counted* storage = alloc.allocate(4);
  for (int i = 0; i < 4; ++i) {
    std::construct_at(storage + i, &destroyed);
  }
  int check = 0;
  switch (which) {
    case 0:
      std::destroy(storage, storage + 4);
      break;
    case 1:
      check = std::destroy_n(storage, 4) == storage + 4 ? 0 : 100;
      break;
    case 2:
      check = std::ranges::destroy(storage, storage + 4) == storage + 4 ? 0 : 100;
      break;
    case 3: {
      std::ranges::subrange<Counted*> all(storage, storage + 4);
      check = std::ranges::destroy(all) == storage + 4 ? 0 : 100;
      break;
    }
    case 4:
      check = std::ranges::destroy_n(storage, 4) == storage + 4 ? 0 : 100;
      break;
    default:
      for (int i = 0; i < 4; ++i) {
        std::ranges::destroy_at(storage + i);
      }
      break;
  }
  alloc.deallocate(storage, 4);
  return destroyed + check;
}

static_assert(destroy_with(0) == 4);
static_assert(destroy_with(1) == 4);
static_assert(destroy_with(2) == 4);
static_assert(destroy_with(3) == 4);
static_assert(destroy_with(4) == 4);
static_assert(destroy_with(5) == 4);

int main() {
  for (int which = 0; which <= 5; ++which) {
    if (destroy_with(which) != 4) {
      return which + 1;
    }
  }
  return 0;
}
