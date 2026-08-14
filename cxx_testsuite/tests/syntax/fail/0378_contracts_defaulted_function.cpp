// RUN: -std=c++26
// EXPECT: a function defaulted on its first declaration cannot have contract assertions

struct value {
  value() pre (true) = default;
};
