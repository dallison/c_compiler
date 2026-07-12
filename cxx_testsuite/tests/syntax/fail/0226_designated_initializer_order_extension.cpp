// RUN: -std=c++17
// EXPECT: designator order for field 'first' does not match declaration order in 'Record'
// EXPECT: '.first' designator used multiple times in the same initializer list

// GCC and Clang apply the declaration-order diagnostic to their designated
// initializer extension in pre-C++20 language modes as well.
struct Record {
  int first;
  int second;
};

Record reversed{.second = 2, .first = 1};
Record duplicate{.first = 1, .first = 2};
