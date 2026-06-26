// RUN: -std=c++20
// EXPECT: Duplicate class member value

struct FunctionUsingA {
  int value(int input);
};

struct FunctionUsingB {
  int value(int input);
};

template <class... Bases>
struct BadFunctionUsingPack : Bases... {
  using Bases::value...;
};

BadFunctionUsingPack<FunctionUsingA, FunctionUsingB> bad_function_using_pack;
