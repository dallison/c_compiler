// RUN: -std=c++20
// EXPECT: Template parameter pack must be last

template <class... Ts, class U>
struct BadPackOrder {
  U value;
};
