// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

volatile int sink;
volatile int caught;

struct Cell {
  int value;
};

__attribute__((noinline)) static int Opaque(int v) {
  return v + sink;
}

__attribute__((noinline)) static void Thrower(int v) {
  throw v;
}

// Every value here is defined before the try and used after the catch, so the
// register allocator keeps it in a callee-saved register across the call that
// throws.  A handler that reloads those registers from its frame gets the
// values its own caller passed in rather than these, and the store through
// `cell` then lands on whatever the caller kept in that register.
__attribute__((noinline)) static int CatchKeepsOwnRegisters(Cell* cell) {
  int a = Opaque(1);
  int b = Opaque(2);
  int c = Opaque(3);
  int d = Opaque(4);
  int e = Opaque(5);
  int f = Opaque(6);
  int g = Opaque(7);
  int h = Opaque(8);
  try {
    Thrower(9);
  } catch (int value) {
    caught = value;
  }
  cell->value = a + b + c + d + e + f + g + h;
  return caught;
}

int main() {
  Cell cell = {0};
  if (CatchKeepsOwnRegisters(&cell) != 9) {
    return 1;
  }
  if (cell.value != 36) {
    return 2;
  }
  return 0;
}
