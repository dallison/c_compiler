// A function that makes no calls but still needs a frame -- it has locals and
// runs out of scratch registers, so callee-saved ones get used -- had its
// register save area placed 8 bytes too high, on top of the last local
// variable.  Restoring the registers on the way out then handed the caller back
// whatever the locals happened to hold.
//
// The x86-64 layout that triggers it wants a local array plus enough live
// values at once to reach the callee-saved registers, and a caller that keeps
// live values of its own in those registers across the call.

template <class... Ts>
int sum_incremented(Ts... args) {
  int values[] = { (args + 1)... };
  return values[0] + values[1] + values[2];
}

template <class... Ts>
int sum_plain(Ts... args) {
  int values[] = { args... };
  return values[0] + values[1] + values[2];
}

int main(void) {
  // The first call is what pushes the caller's arguments into callee-saved
  // registers for the second one, so both are needed.
  if (sum_plain(4, 5, 6) != 15) {
    return 1;
  }
  if (sum_incremented(4, 5, 6) != 18) {
    return 2;
  }
  if (sum_incremented(10, 20, 30) != 63) {
    return 3;
  }
  return 0;
}
