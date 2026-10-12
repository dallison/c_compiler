// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>
#include <utility>

struct PointerWrapper {
  void* pointer;

  PointerWrapper() = default;
  PointerWrapper(void* context, const PointerWrapper& other)
      : pointer(context != nullptr ? context : other.pointer) {}
};

static_assert(std::is_trivially_move_constructible<PointerWrapper>::value);

union Storage {
  constexpr Storage() : integer(0) {}

  int integer;
  PointerWrapper wrapper;
};

static_assert(std::is_move_constructible<Storage>::value);

int main() {
  Storage left;
  Storage right;
  left.integer = 17;
  right.integer = 25;
  std::swap(left, right);
  return left.integer + right.integer == 42 ? 0 : 1;
}
