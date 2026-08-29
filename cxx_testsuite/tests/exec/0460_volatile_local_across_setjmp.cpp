// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// A volatile local has to keep its stored value across a longjmp, which means
// it has to live in memory rather than a register.  x86-64 and RISC-V used to
// bind one to a register when optimizing, and the longjmp restored the
// register to the value it held at the setjmp.

#include <csetjmp>

static int setjmp_keeps_volatile() {
  std::jmp_buf jb;
  volatile int reached = 0;
  int r = setjmp(jb);
  if (r == 0) {
    reached = 1;
    std::longjmp(jb, 42);
  }
  if (reached != 1) return 1;
  if (r != 42) return 2;
  return 0;
}

// A volatile counter must be re-read from memory each time, so an increment
// through a second name for the same object is visible to the next read.
static volatile int counter;

static void bump() { counter = counter + 1; }

static int volatile_reads_reach_memory() {
  counter = 0;
  volatile int local = 0;
  local = local + 1;
  bump();
  bump();
  if (counter != 2) return 3;
  if (local != 1) return 4;
  return 0;
}

int main() {
  int rc = setjmp_keeps_volatile();
  if (rc != 0) return rc;
  return volatile_reads_reach_memory();
}
