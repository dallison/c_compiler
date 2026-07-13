// RUN: -std=c++20
// EXPECT: constraints not satisfied

template <typename T>
concept Never = false;

template <Never auto N>
struct Rejected {
};

Rejected<1> rejected;
