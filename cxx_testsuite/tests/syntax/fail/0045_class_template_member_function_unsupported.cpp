// RUN: -std=c++20
// EXPECT: Class template instantiation is not supported yet

template <typename T>
struct WithMemberFunction {
  T value;
  int get(void);
};

WithMemberFunction<int> first;
WithMemberFunction<int> second;
