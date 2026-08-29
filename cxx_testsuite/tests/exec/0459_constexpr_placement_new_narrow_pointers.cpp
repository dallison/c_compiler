// RUN: -std=c++20
// EXPECT_EXIT: 0

// Constant evaluation runs the function as p-code, where an address is the
// interpreter's own and so wider than a pointer on a 32 bit target.  Two things
// followed from that.  The runtime call behind a placement new reads its
// arguments off the stack, and it read them at eight bytes apiece even where the
// declaration it stands in for has a four byte size_t and a four byte pointer,
// so it took the size for the pointer and rejected the result as unreadable.
// And a pointer written to memory keeps only its low half there, so one read
// back out no longer named the object it pointed at and no longer compared equal
// to a freshly taken address of that object.

#include <expected>
#include <memory>
#include <new>
#include <variant>

static constexpr bool stored_pointer_compares_equal(void) {
  int value = 1;
  int* address = &value;
  return address == &value;
}

static constexpr bool placement_new_returns_its_argument(void) {
  int value = 1;
  int* address = new (&value) int(42);
  return address == &value && value == 42;
}

// The address escapes into a callee, which is where the argument widths matter.
template <class T>
static constexpr T* construct_in(T* where) {
  return ::new (where) T();
}

static constexpr bool placement_new_in_a_callee(void) {
  struct pair {
    char bytes[8];
    int index;
    constexpr pair() : bytes{}, index(7) {}
  };
  pair storage;
  int* constructed = construct_in(reinterpret_cast<int*>(storage.bytes));
  return constructed != nullptr && storage.index == 7;
}

static constexpr bool reconstruct_in_place(void) {
  int value = 1;
  std::destroy_at(&value);
  return std::construct_at(&value, 42) == &value && value == 42;
}

static constexpr int variant_round_trip(void) {
  std::variant<int, double> held(3);
  held = 4.5;
  held = 8;
  return std::get<int>(held);
}

static constexpr int expected_round_trip(void) {
  std::expected<int, int> value(3);
  std::expected<int, int> error(std::unexpect, 4);
  return value.value() + error.error();
}

static_assert(stored_pointer_compares_equal(), "stored pointer");
static_assert(placement_new_returns_its_argument(), "placement new result");
static_assert(placement_new_in_a_callee(), "placement new in a callee");
static_assert(reconstruct_in_place(), "reconstruct in place");
static_assert(variant_round_trip() == 8, "variant");
static_assert(expected_round_trip() == 7, "expected");

int main(void) {
  if (!stored_pointer_compares_equal()) {
    return 1;
  }
  if (!placement_new_returns_its_argument()) {
    return 2;
  }
  if (!placement_new_in_a_callee()) {
    return 3;
  }
  if (!reconstruct_in_place()) {
    return 4;
  }
  if (variant_round_trip() != 8) {
    return 5;
  }
  if (expected_round_trip() != 7) {
    return 6;
  }
  return 0;
}
