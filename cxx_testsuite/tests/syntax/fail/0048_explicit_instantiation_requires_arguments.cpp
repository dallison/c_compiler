// RUN: -std=c++20
// EXPECT: Expected template argument list

template <typename T>
struct Box {
  T value;
};

template struct Box;
