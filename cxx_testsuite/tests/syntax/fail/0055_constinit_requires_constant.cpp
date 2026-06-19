// RUN: -std=c++20
int runtime(void);

constinit int value = runtime();
// EXPECT: constinit variable initializer is not a constant expression
