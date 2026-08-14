// RUN: -std=c++26
// EXPECT: constant evaluation ended with an uncaught exception of type int
// EXPECT: static_assert expression is not an integer constant expression

constexpr int uncaught_exception() {
  throw 99;
}

static_assert(uncaught_exception() == 0);
