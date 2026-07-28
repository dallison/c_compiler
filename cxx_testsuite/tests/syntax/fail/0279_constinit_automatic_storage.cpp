// RUN: -std=c++20
// EXPECT: 'constinit' variable must have static or thread storage duration

int function() {
  constinit int value = 1;
  return value;
}
