// RUN: -std=c++20
// EXPECT: variable declaration in condition must have an initializer
// Recover an uninitialized condition declaration as a declaration, then allow
// the following initialized condition to have an empty loop body.
void test() {
  while (int missing)
    ;
  while (float value = 0)
    ;
}
