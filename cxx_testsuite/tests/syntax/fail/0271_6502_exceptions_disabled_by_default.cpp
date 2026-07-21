// RUN: -target 6502
// EXPECT: cannot use 'throw' with exception handling disabled

void throw_on_6502() {
  throw 1;
}
