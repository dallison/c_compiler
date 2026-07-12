// RUN: -std=c++11
// EXPECT: Non-static data member cannot be thread_local

struct BadThreadLocalMember {
  thread_local int value;
};
