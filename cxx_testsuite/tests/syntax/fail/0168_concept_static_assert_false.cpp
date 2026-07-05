// RUN: -std=c++20
// EXPECT: static assertion failed
template <typename T>
concept Never = false;

static_assert(Never<int>);
