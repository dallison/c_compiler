// RUN: -std=c++20
// EXPECT: Template non-type argument must be an integer constant expression

template <int N>
struct Buffer {
  int data[N];
};

int count;
Buffer<count> object;
