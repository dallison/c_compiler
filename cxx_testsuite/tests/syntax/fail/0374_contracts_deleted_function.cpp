// RUN: -std=c++26
// EXPECT: deleted functions cannot have contract assertions

int invalid() pre (true) = delete;
