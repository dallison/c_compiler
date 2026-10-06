// RUN: -std=c++20 -fconstexpr-eval=pcode
// EXPECT: bit_cast of a pointer, reference, union or volatile subobject is not a constant expression

// A pointer's representation is not available to a constant expression.
#include <bit>

int global;

struct Holder {
  int* pointer;
};

constexpr unsigned long address_bits() {
  Holder holder{&global};
  return std::bit_cast<unsigned long>(holder);
}

static_assert(address_bits() != 0);
