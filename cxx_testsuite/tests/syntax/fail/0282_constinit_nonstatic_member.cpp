// RUN: -std=c++20
// EXPECT: 'constinit' variable must have static or thread storage duration

struct value {
  constinit int member;
};
