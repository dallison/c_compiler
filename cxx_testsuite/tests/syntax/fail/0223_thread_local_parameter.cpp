// RUN: -std=c++11
// EXPECT: Illegal use of thread_local

int f(thread_local int value) {
  return value;
}
