// RUN: -std=c++11
// EXPECT: Illegal use of thread_local

thread_local int f(void) {
  return 0;
}
