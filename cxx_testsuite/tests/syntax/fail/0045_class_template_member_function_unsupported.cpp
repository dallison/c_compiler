// RUN: -std=c++20
// EXPECT: Class template instantiation is not supported yet

template <typename T>
struct WithStaticMemberFunction {
  T value;
  static int get(void);
};

WithStaticMemberFunction<int> first;
WithStaticMemberFunction<int> second;
