// RUN: -std=c++20 -O2

// Catching an exception inside a called frame must leave the caller's
// callee-saved registers alone.  A landing pad is re-entered with the stack
// pointer the throwing call site left it at, which is below the frame because
// the outgoing arguments were pushed there, so a frame that reloads its saved
// registers relative to the stack pointer handed the caller the arguments of
// the call that threw instead of the caller's registers.

volatile int sink;
volatile int caught;

__attribute__((noinline)) static int Opaque(int v) {
  return v + sink;
}

__attribute__((noinline)) static void Thrower(int v) {
  throw v;
}

// The catch has to be in a frame that pushed arguments for the call that
// throws, and that uses callee-saved registers of its own.
__attribute__((noinline)) static void CatchInside(int v) {
  int local = Opaque(v);
  try {
    Thrower(local);
  } catch (int) {
    caught += Opaque(1) - sink;
  }
}

int main() {
  // Enough live values across the call for some of them to be in callee-saved
  // registers rather than spilled.
  int a = Opaque(1);
  int b = Opaque(2);
  int c = Opaque(3);
  int d = Opaque(4);
  int e = Opaque(5);
  int f = Opaque(6);
  int g = Opaque(7);
  int h = Opaque(8);

  CatchInside(9);
  if (caught != 1) {
    return 1;
  }
  if (a + b + c + d + e + f + g + h != 36) {
    return 2;
  }

  // Repeatedly comparing against the same constant tends to park that constant
  // in a callee-saved register across the calls in between.
  if (sink != 0) {
    return 3;
  }
  CatchInside(9);
  if (sink != 0) {
    return 4;
  }
  CatchInside(9);
  if (sink != 0) {
    return 5;
  }
  if (caught != 3) {
    return 6;
  }
  if (a + b + c + d + e + f + g + h != 36) {
    return 7;
  }
  return 0;
}
