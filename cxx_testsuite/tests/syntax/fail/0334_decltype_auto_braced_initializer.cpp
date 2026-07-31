// RUN: -std=c++20
// EXPECT: Cannot deduce decltype(auto) from braced initializer

decltype(auto) value = {1};
