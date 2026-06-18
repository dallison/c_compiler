// RUN: -std=c++20

template <typename T>
struct WithStaticMemberFunction {
  T value;
  static int get(void);
};

WithStaticMemberFunction<int> first;
WithStaticMemberFunction<int> second;
