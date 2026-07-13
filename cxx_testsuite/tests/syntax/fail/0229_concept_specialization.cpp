// RUN: -std=c++20
// EXPECT: concept specialization is not permitted

template <typename T>
concept Always = true;

template <>
concept Always<int> = false;
