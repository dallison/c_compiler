// RUN: -std=c++17
// EXPECT: concept definitions require C++20
template <typename T>
concept C = true;
